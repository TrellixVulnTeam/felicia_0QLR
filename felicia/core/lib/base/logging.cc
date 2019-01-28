// Copyright (c) 2012 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// =====================================================================
// Modifications copyright (C) 2019 felicia

#include "felicia/core/lib/base/logging.h"

#include <limits.h>
#include <stdint.h>

#include "absl/strings/str_format.h"

#if defined(PLATFORM_WINDOWS)
#include <io.h>
#include <windows.h>
typedef HANDLE FileHandle;
typedef HANDLE MutexHandle;
// Windows warns on using write().  It prefers _write().
#define write(fd, buf, count) _write(fd, buf, static_cast<unsigned int>(count))
// Windows doesn't define STDERR_FILENO.  Define it here.
#define STDERR_FILENO 2

#elif defined(PLATFORM_MACOSX)
// In MacOS 10.12 and iOS 10.0 and later ASL (Apple System Log) was deprecated
// in favor of OS_LOG (Unified Logging).
#include <AvailabilityMacros.h>
#if defined(PLATFORM_IOS)
#if !defined(__IPHONE_10_0) || __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_10_0
#define USE_ASL
#endif
#else  // !defined(PLATFORM_IOS)
#if !defined(MAC_OS_X_VERSION_10_12) || \
    MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12
#define USE_ASL
#endif
#endif  // defined(PLATFORM_IOS)

#if defined(USE_ASL)
#include <asl.h>
#else
#include <os/log.h>
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#elif defined(PLATFORM_POSIX)
#include <time.h>
#endif

#if defined(PLATFORM_ANDROID)
#include <android/log.h>
#endif

#if defined(PLATFORM_POSIX)
#include <paths.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define MAX_PATH PATH_MAX
typedef FILE* FileHandle;
typedef pthread_mutex_t* MutexHandle;
#endif

#include <algorithm>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <string>
#include <utility>

#include "felicia/core/lib/base/stl_util.h"
#include "felicia/core/lib/debug/alias.h"
#include "felicia/core/platform/lock_impl.h"
#include "felicia/core/platform/platform_thread.h"
#include "felicia/core/platform/posix/eintr_wrapper.h"
#include "felicia/core/platform/sys_string_conversions.h"

namespace logging {

namespace {

const char* const log_severity_names[] = {"INFO", "WARNING", "ERROR", "FATAL"};
static_assert(LOG_NUM_SEVERITIES == felicia::size(log_severity_names),
              "Incorrect number of log_severity_names");

const char* log_severity_name(int severity) {
  if (severity >= 0 && severity < LOG_NUM_SEVERITIES)
    return log_severity_names[severity];
  return "UNKNOWN";
}

int g_min_log_level = 0;

LoggingDestination g_logging_destination = LOG_DEFAULT;

// For LOG_ERROR and above, always print to stderr.
const int kAlwaysPrintErrorLevel = LOG_ERROR;

std::string* g_log_file_name = nullptr;

// This file is lazily opened and the handle may be nullptr
FileHandle g_log_file = nullptr;

// What should be prepended to each message?
bool g_log_process_id = false;
bool g_log_thread_id = false;
bool g_log_timestamp = true;
bool g_log_tickcount = false;

int32_t CurrentProcessId() {
#if defined(PLATFORM_WINDOWS)
  return GetCurrentProcessId();
#elif defined(PLATFORM_POSIX)
  return getpid();
#endif
}

uint64_t TickCount() {
#if defined(PLATFORM_WINDOWS)
  return GetTickCount();
#elif defined(PLATFORM_MACOSX)
  return mach_absolute_time();
#elif defined(PLATFORM_POSIX)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64_t absolute_micro = static_cast<int64_t>(ts.tv_sec) * 1000000 +
                            static_cast<int64_t>(ts.tv_nsec) / 1000;

  return absolute_micro;
#endif
}

void DeleteFilePath(const std::string& log_name) {
#if defined(PLATFORM_WINDOWS)
  DeleteFile(log_name.c_str());
#elif defined(PLATFORM_POSIX)
  unlink(log_name.c_str());
#else
#error Unsupported platform
#endif
}

std::string GetDefaultLogFile() {
#if defined(PLATFORM_WINDOWS)
  // On Windows we use the same path as the exe.
  wchar_t module_name[MAX_PATH];
  GetModuleFileName(nullptr, module_name, MAX_PATH);

  std::string log_name = module_name;
  std::string::size_type last_backslash = log_name.rfind('\\', log_name.size());
  if (last_backslash != std::string::npos) log_name.erase(last_backslash + 1);
  log_name += L"debug.log";
  return log_name;
#elif defined(PLATFORM_POSIX)
  // On other platforms we just use the current directory.
  return std::string("debug.log");
#endif
}

// We don't need locks on Windows for atomically appending to files. The OS
// provides this functionality.
#if defined(PLATFORM_POSIX)
// This class acts as a wrapper for locking the logging files.
// LoggingLock::Init() should be called from the main thread before any logging
// is done. Then whenever logging, be sure to have a local LoggingLock
// instance on the stack. This will ensure that the lock is unlocked upon
// exiting the frame.
// LoggingLocks can not be nested.
class LoggingLock {
 public:
  LoggingLock() { LockLogging(); }

  ~LoggingLock() { UnlockLogging(); }

  static void Init(LogLockingState lock_log, const std::string& new_log_file) {
    if (initialized) return;
    lock_log_file = lock_log;

    if (lock_log_file != LOCK_LOG_FILE)
      log_lock = new felicia::internal::LockImpl();

    initialized = true;
  }

 private:
  static void LockLogging() {
    if (lock_log_file == LOCK_LOG_FILE) {
      pthread_mutex_lock(&log_mutex);
    } else {
      // use the lock
      log_lock->Lock();
    }
  }

  static void UnlockLogging() {
    if (lock_log_file == LOCK_LOG_FILE) {
      pthread_mutex_unlock(&log_mutex);
    } else {
      log_lock->Unlock();
    }
  }

  // The lock is used if log file locking is false. It helps us avoid problems
  // with multiple threads writing to the log file at the same time.  Use
  // LockImpl directly instead of using Lock, because Lock makes logging calls.
  static felicia::internal::LockImpl* log_lock;

  // When we don't use a lock, we are using a global mutex. We need to do this
  // because LockFileEx is not thread safe.
  static pthread_mutex_t log_mutex;

  static bool initialized;
  static LogLockingState lock_log_file;
};

// static
bool LoggingLock::initialized = false;
// static
felicia::internal::LockImpl* LoggingLock::log_lock = nullptr;
// static
LogLockingState LoggingLock::lock_log_file = LOCK_LOG_FILE;

pthread_mutex_t LoggingLock::log_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif  // PLATFORM_POSIX

// Called by logging functions to ensure that |g_log_file| is initialized
// and can be used for writing. Returns false if the file could not be
// initialized. |g_log_file| will be nullptr in this case.
bool InitializeLogFileHandle() {
  if (g_log_file) return true;

  if (!g_log_file_name) {
    // Nobody has called InitLogging to specify a debug log file, so here we
    // initialize the log file name to a default.
    g_log_file_name = new std::string(GetDefaultLogFile());
  }

  if ((g_logging_destination & LOG_TO_FILE) != 0) {
#if defined(PLATFORM_WINDOWS)
    // The FILE_APPEND_DATA access mask ensures that the file is atomically
    // appended to across accesses from multiple threads.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa364399(v=vs.85).aspx
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
    g_log_file = CreateFile(g_log_file_name->c_str(), FILE_APPEND_DATA,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (g_log_file == INVALID_HANDLE_VALUE || g_log_file == nullptr) {
      // We are intentionally not using FilePath or FileUtil here to reduce the
      // dependencies of the logging implementation. For e.g. FilePath and
      // FileUtil depend on shell32 and user32.dll. This is not acceptable for
      // some consumers of base logging like chrome_elf, etc.
      // Please don't change the code below to use FilePath.
      // try the current directory
      wchar_t system_buffer[MAX_PATH];
      system_buffer[0] = 0;
      DWORD len =
          ::GetCurrentDirectory(arraysize(system_buffer), system_buffer);
      if (len == 0 || len > arraysize(system_buffer)) return false;

      *g_log_file_name = system_buffer;
      // Append a trailing backslash if needed.
      if (g_log_file_name->back() != L'\\') *g_log_file_name += L"\\";
      *g_log_file_name += L"debug.log";

      g_log_file = CreateFile(g_log_file_name->c_str(), FILE_APPEND_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (g_log_file == INVALID_HANDLE_VALUE || g_log_file == nullptr) {
        g_log_file = nullptr;
        return false;
      }
    }
#elif defined(PLATFORM_POSIX)
    g_log_file = fopen(g_log_file_name->c_str(), "a");
    if (g_log_file == nullptr) return false;
#else
#error Unsupported platform
#endif
  }

  return true;
}

void CloseFile(FileHandle log) {
#if defined(PLATFORM_WINDOWS)
  CloseHandle(log);
#elif defined(PLATFORM_POSIX)
  fclose(log);
#else
#error Unsupported platform
#endif
}

void CloseLogFileUnlocked() {
  if (!g_log_file) return;

  CloseFile(g_log_file);
  g_log_file = nullptr;
}

}  // namespace

#if defined(DHECK_IS_CONFIGURABLE)
// In DCHECK-enabled Chrome builds, allow the meaning of LOG_DCHECK to be
// determined at run-time. We default it to INFO, to avoid it triggering
// crashes before the run-time has explicitly chosen the behaviour.
EXPORT logging::LogSeverity LOG_DCHECK = LOG_INFO;
#endif  // defined(DHECK_IS_CONFIGURABLE)

// This is never instantiated, it's just used for EAT_STREAM_PARAMETERS to have
// an object of the correct type on the LHS of the unused part of the ternary
// operator.
std::ostream* g_swallow_stream;

LoggingSettings::LoggingSettings()
    : logging_dest(LOG_DEFAULT),
      log_file(nullptr),
      lock_log(LOCK_LOG_FILE),
      delete_old(APPEND_TO_OLD_LOG_FILE) {}

bool ShouldCreateLogMessage(int severity) {
  if (severity < g_min_log_level) return false;

  // Return true here unless we know ~LogMessage won't do anything. Note that
  // ~LogMessage writes to stderr if severity_ >= kAlwaysPrintErrorLevel, even
  // when g_logging_destination is LOG_NONE.
  return g_logging_destination != LOG_NONE ||
         severity >= kAlwaysPrintErrorLevel;
}

// Explicit instantiations for commonly used comparisons.
template std::string* MakeCheckOpString<int, int>(const int&, const int&,
                                                  const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
template std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);

void MakeCheckOpValueString(std::ostream* os, std::nullptr_t p) {
  (*os) << "nullptr";
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, const char* condition)
    : severity_(LOG_FATAL), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << condition << ". ";
}

LogMessage::LogMessage(const char* file, int line, std::string* result)
    : severity_(LOG_FATAL), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       std::string* result)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::~LogMessage() {
  size_t stack_start = stream_.tellp();
  /*
#if !defined(OFFICIAL_BUILD) && !defined(__UCLIBC__)
  if (severity_ == LOG_FATAL && !base::debug::BeingDebugged()) {
    // Include a stack trace on a fatal, unless a debugger is attached.
    base::debug::StackTrace trace;
    stream_ << std::endl;  // Newline to separate from log message.
    trace.OutputToStream(&stream_);
  }
#endif
  */
  stream_ << std::endl;
  std::string str_newline(stream_.str());

  if (severity_ == LOG_FATAL /* && !felicia::debug::BeingDebugged() */) {
    // Include a stack trace on a fatal, unless a debugger is attached.
    // TODO(chokobole): Implement Stacktrace with chromium
    // felicia::debug::StackTrace trace;
    // stream_ << std::endl;  // Newline to separate from log message.
    // trace.OutputToStream(&stream_);
  }

  if ((g_logging_destination & LOG_TO_SYSTEM_DEBUG_LOG) != 0) {
#if defined(PLATFORM_WINDOWS)
    OutputDebugStringA(str_newline.c_str());
#elif defined(PLATFORM_MACOSX)
    // In LOG_TO_SYSTEM_DEBUG_LOG mode, log messages are always written to
    // stderr. If stderr is /dev/null, also log via ASL (Apple System Log) or
    // its successor OS_LOG. If there's something weird about stderr, assume
    // that log messages are going nowhere and log via ASL/OS_LOG too.
    // Messages logged via ASL/OS_LOG show up in Console.app.
    //
    // Programs started by launchd, as UI applications normally are, have had
    // stderr connected to /dev/null since OS X 10.8. Prior to that, stderr was
    // a pipe to launchd, which logged what it received (see log_redirect_fd in
    // 10.7.5 launchd-392.39/launchd/src/launchd_core_logic.c).
    //
    // Another alternative would be to determine whether stderr is a pipe to
    // launchd and avoid logging via ASL only in that case. See 10.7.5
    // CF-635.21/CFUtilities.c also_do_stderr(). This would result in logging to
    // both stderr and ASL/OS_LOG even in tests, where it's undesirable to log
    // to the system log at all.
    //
    // Note that the ASL client by default discards messages whose levels are
    // below ASL_LEVEL_NOTICE. It's possible to change that with
    // asl_set_filter(), but this is pointless because syslogd normally applies
    // the same filter.
    const bool log_to_system = []() {
      struct stat stderr_stat;
      if (fstat(fileno(stderr), &stderr_stat) == -1) {
        return true;
      }
      if (!S_ISCHR(stderr_stat.st_mode)) {
        return false;
      }

      struct stat dev_null_stat;
      if (stat(_PATH_DEVNULL, &dev_null_stat) == -1) {
        return true;
      }

      return !S_ISCHR(dev_null_stat.st_mode) ||
             stderr_stat.st_rdev == dev_null_stat.st_rdev;
    }();

    if (log_to_system) {
      // Log roughly the same way that CFLog() and NSLog() would. See 10.10.5
      // CF-1153.18/CFUtilities.c __CFLogCString().
      CFBundleRef main_bundle = CFBundleGetMainBundle();
      CFStringRef main_bundle_id_cf =
          main_bundle ? CFBundleGetIdentifier(main_bundle) : nullptr;
      std::string main_bundle_id =
          main_bundle_id_cf
              ? felicia::mac::SysCFStringRefToUTF8(main_bundle_id_cf)
              : std::string("");
#if defined(USE_ASL)
      // The facility is set to the main bundle ID if available. Otherwise,
      // "com.apple.console" is used.
      const class ASLClient {
       public:
        explicit ASLClient(const std::string& facility)
            : client_(asl_open(nullptr, facility.c_str(), ASL_OPT_NO_DELAY)) {}
        ~ASLClient() { asl_close(client_); }

        aslclient get() const { return client_; }

       private:
        aslclient client_;
        DISALLOW_COPY_AND_ASSIGN(ASLClient);
      } asl_client(main_bundle_id.empty() ? main_bundle_id
                                          : "com.apple.console");

      const class ASLMessage {
       public:
        ASLMessage() : message_(asl_new(ASL_TYPE_MSG)) {}
        ~ASLMessage() { asl_free(message_); }

        aslmsg get() const { return message_; }

       private:
        aslmsg message_;
        DISALLOW_COPY_AND_ASSIGN(ASLMessage);
      } asl_message;

      // By default, messages are only readable by the admin group. Explicitly
      // make them readable by the user generating the messages.
      char euid_string[12];
      snprintf(euid_string, arraysize(euid_string), "%d", geteuid());
      asl_set(asl_message.get(), ASL_KEY_READ_UID, euid_string);

      // Map Chrome log severities to ASL log levels.
      const char* const asl_level_string = [](LogSeverity severity) {
      // ASL_LEVEL_* are ints, but ASL needs equivalent strings. This
      // non-obvious two-step macro trick achieves what's needed.
      // https://gcc.gnu.org/onlinedocs/cpp/Stringification.html
#define ASL_LEVEL_STR(level) ASL_LEVEL_STR_X(level)
#define ASL_LEVEL_STR_X(level) #level
        switch (severity) {
          case LOG_INFO:
            return ASL_LEVEL_STR(ASL_LEVEL_INFO);
          case LOG_WARNING:
            return ASL_LEVEL_STR(ASL_LEVEL_WARNING);
          case LOG_ERROR:
            return ASL_LEVEL_STR(ASL_LEVEL_ERR);
          case LOG_FATAL:
            return ASL_LEVEL_STR(ASL_LEVEL_CRIT);
          default:
            return severity < 0 ? ASL_LEVEL_STR(ASL_LEVEL_DEBUG)
                                : ASL_LEVEL_STR(ASL_LEVEL_NOTICE);
        }
#undef ASL_LEVEL_STR
#undef ASL_LEVEL_STR_X
      }(severity_);
      asl_set(asl_message.get(), ASL_KEY_LEVEL, asl_level_string);

      asl_set(asl_message.get(), ASL_KEY_MSG, str_newline.c_str());

      asl_send(asl_client.get(), asl_message.get());
#else   // !defined(USE_ASL)
      const class OSLog {
       public:
        explicit OSLog(const char* subsystem)
            : os_log_(subsystem ? os_log_create(subsystem, "felicia_logging")
                                : OS_LOG_DEFAULT) {}
        ~OSLog() {
          if (os_log_ != OS_LOG_DEFAULT) {
            os_release(os_log_);
          }
        }
        os_log_t get() const { return os_log_; }

       private:
        os_log_t os_log_;
        DISALLOW_COPY_AND_ASSIGN(OSLog);
      } log(main_bundle_id.empty() ? nullptr : main_bundle_id.c_str());
      const os_log_type_t os_log_type = [](LogSeverity severity) {
        switch (severity) {
          case LOG_INFO:
            return OS_LOG_TYPE_INFO;
          case LOG_WARNING:
            return OS_LOG_TYPE_DEFAULT;
          case LOG_ERROR:
            return OS_LOG_TYPE_ERROR;
          case LOG_FATAL:
            return OS_LOG_TYPE_FAULT;
          default:
            return severity < 0 ? OS_LOG_TYPE_DEBUG : OS_LOG_TYPE_DEFAULT;
        }
      }(severity_);
      os_log_with_type(log.get(), os_log_type, "%{public}s",
                       str_newline.c_str());
#endif  // defined(USE_ASL)
    }
#elif defined(PLATFORM_ANDROID)
    android_LogPriority priority =
        (severity_ < 0) ? ANDROID_LOG_VERBOSE : ANDROID_LOG_UNKNOWN;
    switch (severity_) {
      case LOG_INFO:
        priority = ANDROID_LOG_INFO;
        break;
      case LOG_WARNING:
        priority = ANDROID_LOG_WARN;
        break;
      case LOG_ERROR:
        priority = ANDROID_LOG_ERROR;
        break;
      case LOG_FATAL:
        priority = ANDROID_LOG_FATAL;
        break;
    }
#if DCHECK_IS_ON()
    // Split the output by new lines to prevent the Android system from
    // truncating the log.
    for (const auto& line : absl::StrSplit(str_newline, "\n"))
      __android_log_write(priority, "felicia", line.c_str());
#else
    // The Android system may truncate the string if it's too long.
    __android_log_write(priority, "felicia", str_newline.c_str());
#endif
#endif  // PLATFORM_ANDROID
    ignore_result(fwrite(str_newline.data(), str_newline.size(), 1, stderr));
    fflush(stderr);
  } else if (severity_ >= kAlwaysPrintErrorLevel) {
    // When we're only outputting to a log file, above a certain log level, we
    // should still output to stderr so that we can better detect and diagnose
    // problems with unit tests, especially on the buildbots.
    ignore_result(fwrite(str_newline.data(), str_newline.size(), 1, stderr));
    fflush(stderr);
  }

  // write to log file
  if ((g_logging_destination & LOG_TO_FILE) != 0) {
    // We can have multiple threads and/or processes, so try to prevent them
    // from clobbering each other's writes.
    // If the client app did not call InitLogging, and the lock has not
    // been created do it now. We do this on demand, but if two threads try
    // to do this at the same time, there will be a race condition to create
    // the lock. This is why InitLogging should be called from the main
    // thread at the beginning of execution.
#if defined(PLATFORM_POSIX)
    LoggingLock::Init(LOCK_LOG_FILE, nullptr);
    LoggingLock logging_lock;
#endif
    if (InitializeLogFileHandle()) {
#if defined(PLATFORM_WINDOWS)
      DWORD num_written;
      WriteFile(g_log_file, static_cast<const void*>(str_newline.c_str()),
                static_cast<DWORD>(str_newline.length()), &num_written,
                nullptr);
#elif defined(PLATFORM_POSIX)
      ignore_result(
          fwrite(str_newline.data(), str_newline.size(), 1, g_log_file));
      fflush(g_log_file);
#else
#error Unsupported platform
#endif
    }
  }

  if (severity_ == LOG_FATAL) {
    // Ensure the first characters of the string are on the stack so they
    // are contained in minidumps for diagnostic purposes. We place start
    // and end marker values at either end, so we can scan captured stacks
    // for the data easily.
    struct {
      uint32_t start_marker = 0xbedead01;
      char data[1024];
      uint32_t end_marker = 0x5050dead;
    } str_stack;
    felicia::strlcpy(str_stack.data, str_newline.data(),
                     felicia::size(str_stack.data));
    felicia::debug::Alias(&str_stack);

    // Crash the process to generate a dump.
#if defined(NDEBUG)
    IMMEDIATE_CRASH();
#else
    // base::debug::BreakDebugger();
    IMMEDIATE_CRASH();
#endif
  }
}

// writes the common header info to the stream
void LogMessage::Init(const char* file, int line) {
  absl::string_view filename(file);
  size_t last_slash_pos = filename.find_last_of("\\/");
  if (last_slash_pos != std::string::npos)
    filename.remove_prefix(last_slash_pos + 1);

  // TODO(darin): It might be nice if the columns were fixed width.

  stream_ << '[';
  if (g_log_process_id) stream_ << CurrentProcessId() << ':';
  if (g_log_thread_id) stream_ << felicia::PlatformThread::CurrentId() << ':';
  if (g_log_timestamp) {
#if defined(PLATFORM_WINDOWS)
    SYSTEMTIME local_time;
    GetLocalTime(&local_time);
    stream_ << std::setfill('0') << std::setw(2) << local_time.wMonth
            << std::setw(2) << local_time.wDay << '/' << std::setw(2)
            << local_time.wHour << std::setw(2) << local_time.wMinute
            << std::setw(2) << local_time.wSecond << '.' << std::setw(3)
            << local_time.wMilliseconds << ':';
#elif defined(PLATFORM_POSIX)
    timeval tv;
    gettimeofday(&tv, nullptr);
    time_t t = tv.tv_sec;
    struct tm local_time;
    localtime_r(&t, &local_time);
    struct tm* tm_time = &local_time;
    stream_ << std::setfill('0') << std::setw(2) << 1 + tm_time->tm_mon
            << std::setw(2) << tm_time->tm_mday << '/' << std::setw(2)
            << tm_time->tm_hour << std::setw(2) << tm_time->tm_min
            << std::setw(2) << tm_time->tm_sec << '.' << std::setw(6)
            << tv.tv_usec << ':';
#else
#error Unsupported platform
#endif
  }
  if (g_log_tickcount) stream_ << TickCount() << ':';
  if (severity_ >= 0)
    stream_ << log_severity_name(severity_);
  else
    stream_ << "VERBOSE" << -severity_;

  stream_ << ":" << filename << "(" << line << ")] ";
}

#if defined(PLATFORM_WINDOWS)
Win32ErrorLogMessage::Win32ErrorLogMessage(const char* file, int line,
                                           LogSeverity severity,
                                           PlatformErrorCode err)
    : err_(err), log_message_(file, line, severity) {}

Win32ErrorLogMessage::~Win32ErrorLogMessage() {
  stream() << ": " << ::felicia::PlatformErrorCodeToString(err_);
  // We're about to crash (CHECK). Put |err_| on the stack (by placing it in a
  // field) and use Alias in hopes that it makes it into crash dumps.
  DWORD last_error = err_;
  felicia::debug::Alias(&last_error);
}
#elif defined(PLATFORM_POSIX)
ErrnoLogMessage::ErrnoLogMessage(const char* file, int line,
                                 LogSeverity severity, PlatformErrorCode err)
    : err_(err), log_message_(file, line, severity) {}

ErrnoLogMessage::~ErrnoLogMessage() {
  stream() << ": " << ::felicia::PlatformErrorCodeToString(err_);
  // We're about to crash (CHECK). Put |err_| on the stack (by placing it in a
  // field) and use Alias in hopes that it makes it into crash dumps.
  int last_error = err_;
  felicia::debug::Alias(&last_error);
}
#endif  // defined(PLATFORM_WINDOWS)

EXPORT void LogErrorNotReached(const char* file, int line) {
  LogMessage(file, line, LOG_ERROR).stream() << "NOTREACHED() hit.";
}

}  // namespace logging