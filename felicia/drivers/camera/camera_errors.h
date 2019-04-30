#ifndef FELICIA_DRIVERS_CAMERA_CAMERA_ERRORS_H_
#define FELICIA_DRIVERS_CAMERA_CAMERA_ERRORS_H_

#include "third_party/chromium/base/strings/string_util.h"
#include "third_party/chromium/base/strings/stringprintf.h"

#include "felicia/core/lib/error/errors.h"
#include "felicia/drivers/camera/camera_format.h"

namespace felicia {
namespace errors {

inline ::felicia::Status FailedToGetCameraFormat() {
  return Unavailable("Failed to get camera format.");
}

inline ::felicia::Status FailedToGetFrameRate() {
  return Unavailable("Failed to get frame rate.");
}

inline ::felicia::Status FailedToSetPixelFormat() {
  return Unavailable("Failed to set pixel format");
}

inline ::felicia::Status FailedToSetFrameRate() {
  return Unavailable("Failed to set frame rate.");
}

inline ::felicia::Status FailedToConvertToARGB() {
  return Unavailable(::base::StringPrintf("Failed To convert to ARGB."));
}

}  // namespace errors
}  // namespace felicia

#include "third_party/chromium/build/build_config.h"

#if defined(OS_LINUX)
#include "felicia/drivers/camera/linux/v4l2_camera_errors.h"
#elif defined(OS_WIN)
#include "felicia/drivers/camera/win/dshow_camera_errors.h"
#endif

#endif  // FELICIA_DRIVERS_CAMERA_CAMERA_ERRORS_H_