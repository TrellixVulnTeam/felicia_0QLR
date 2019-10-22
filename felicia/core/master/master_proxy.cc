#include "felicia/core/master/master_proxy.h"

#include <csignal>

#include "third_party/chromium/base/logging.h"
#include "third_party/chromium/base/strings/stringprintf.h"

#include "felicia/core/channel/channel_factory.h"
#include "felicia/core/lib/felicia_env.h"
#include "felicia/core/lib/net/net_util.h"
#include "felicia/core/lib/strings/str_util.h"

#if defined(FEL_WIN_NO_GRPC)
namespace felicia {
extern std::unique_ptr<MasterClientInterface> NewMasterClient();
}  // namespace felicia
#else
#include "felicia/core/master/rpc/master_client.h"
#include "felicia/core/master/rpc/master_server_info.h"
#include "felicia/core/rpc/grpc_util.h"
#endif

namespace felicia {

namespace {

bool g_on_background = false;

void StopMasterProxy(int signal) {
  MasterProxy& master_proxy = MasterProxy::GetInstance();
  master_proxy.Stop();
}

}  // namespace

MasterProxy::MasterProxy()
#if defined(OS_WIN)
    : scoped_com_initializer_(base::win::ScopedCOMInitializer::kMTA)
#endif
{
  if (g_on_background) {
    thread_ = std::make_unique<base::Thread>("MasterProxy");
  } else {
    message_loop_ =
        std::make_unique<base::MessageLoop>(base::MessageLoop::TYPE_IO);
    run_loop_ = std::make_unique<base::RunLoop>();
  }

  protobuf_loader_ =
      ProtobufLoader::Load(base::FilePath(FILE_PATH_LITERAL("") FELICIA_ROOT));
}

MasterProxy::~MasterProxy() = default;

// static
void MasterProxy::SetBackground() { g_on_background = true; }

// static
MasterProxy& MasterProxy::GetInstance() {
  static base::NoDestructor<MasterProxy> master_proxy;
  return *master_proxy;
}

ProtobufLoader* MasterProxy::protobuf_loader() {
  return protobuf_loader_.get();
}

const ClientInfo& MasterProxy::client_info() const { return client_info_; }

void MasterProxy::set_heart_beat_duration(base::TimeDelta heart_beat_duration) {
  client_info_.set_heart_beat_duration(heart_beat_duration.InMilliseconds());
}

void MasterProxy::set_on_stop_callback(base::OnceClosure callback) {
  on_stop_callback_ = std::move(callback);
}

bool MasterProxy::IsBoundToCurrentThread() const {
  if (g_on_background) {
    return thread_->task_runner()->BelongsToCurrentThread();
  } else {
    return message_loop_->IsBoundToCurrentThread();
  }
}

bool MasterProxy::PostTask(const base::Location& from_here,
                           base::OnceClosure callback) {
  if (g_on_background) {
    return thread_->task_runner()->PostTask(from_here, std::move(callback));
  } else {
    return message_loop_->task_runner()->PostTask(from_here,
                                                  std::move(callback));
  }
}

bool MasterProxy::PostDelayedTask(const base::Location& from_here,
                                  base::OnceClosure callback,
                                  base::TimeDelta delay) {
  if (g_on_background) {
    return thread_->task_runner()->PostDelayedTask(from_here,
                                                   std::move(callback), delay);
  } else {
    return message_loop_->task_runner()->PostDelayedTask(
        from_here, std::move(callback), delay);
  }
}

#if defined(FEL_WIN_NO_GRPC)
Status MasterProxy::StartMasterClient() {
  master_client_interface_ = NewMasterClient();
  return master_client_interface_->Start();
}

bool MasterProxy::is_client_info_set() const { return is_client_info_set_; }
#endif

Status MasterProxy::Start() {
#if !defined(FEL_WIN_NO_GRPC)
  std::string ip = ResolveMasterServerIp().ToString();
  uint16_t port = ResolveMasterServerPort();
  auto channel = ConnectToGrpcServer(ip, port);
  master_client_interface_ = std::make_unique<MasterClient>(channel);
  Status s = master_client_interface_->Start();
  if (!s.ok()) return s;
#endif

  if (g_on_background) {
    thread_->StartWithOptions(
        base::Thread::Options{base::MessageLoop::TYPE_IO, 0});
  }

  base::WaitableEvent* event = new base::WaitableEvent;
  Setup(event);

  event->Wait();
  delete event;

  RegisterClient();

  return Status::OK();
}

Status MasterProxy::Stop() {
  Status s = master_client_interface_->Stop();
  if (g_on_background) {
    thread_->Stop();
  } else {
    run_loop_->Quit();
  }
  if (!on_stop_callback_.is_null()) std::move(on_stop_callback_).Run();
  return s;
}

#define CLIENT_METHOD(method)                                     \
  void MasterProxy::method##Async(const method##Request* request, \
                                  method##Response* response,     \
                                  StatusOnceCallback done) {      \
    master_client_interface_->method##Async(request, response,    \
                                            std::move(done));     \
  }

CLIENT_METHOD(RegisterClient)
CLIENT_METHOD(ListClients)
CLIENT_METHOD(RegisterNode)
CLIENT_METHOD(UnregisterNode)
CLIENT_METHOD(ListNodes)
CLIENT_METHOD(PublishTopic)
CLIENT_METHOD(UnpublishTopic)
CLIENT_METHOD(SubscribeTopic)
// UnsubscribeTopic needs additional remove callback from
// |master_notification_watcher_|
// CLIENT_METHOD(UnsubscribeTopic)
CLIENT_METHOD(ListTopics)
CLIENT_METHOD(RegisterServiceClient)
// UnregisterServiceClient needs additional remove callback from
// |master_notification_watcher_|
// CLIENT_METHOD(UnregisterServiceClient)
CLIENT_METHOD(RegisterServiceServer)
CLIENT_METHOD(UnregisterServiceServer)
CLIENT_METHOD(ListServices)

#undef CLIENT_METHOD

void MasterProxy::Run() {
  RegisterSignals();
  if (g_on_background) return;
  run_loop_->Run();
}

void MasterProxy::Setup(base::WaitableEvent* event) {
  if (!IsBoundToCurrentThread()) {
    PostTask(FROM_HERE, base::BindOnce(&MasterProxy::Setup,
                                       base::Unretained(this), event));
    return;
  }

  master_notification_watcher_.Start();
  *client_info_.mutable_master_notification_watcher_source() =
      master_notification_watcher_.channel_source();
  heart_beat_signaller_.Start(
      client_info_, base::BindOnce(&MasterProxy::OnHeartBeatSignallerStart,
                                   base::Unretained(this), event));
}

void MasterProxy::RegisterSignals() {
  // To handle general case when POSIX ask the process to quit.
  std::signal(SIGTERM, &felicia::StopMasterProxy);
  // To handle Ctrl + C.
  std::signal(SIGINT, &felicia::StopMasterProxy);
#if defined(OS_POSIX)
  // To handle when the terminal is closed.
  std::signal(SIGHUP, &felicia::StopMasterProxy);
#endif
}

void MasterProxy::OnHeartBeatSignallerStart(
    base::WaitableEvent* event, const ChannelSource& channel_source) {
  *client_info_.mutable_heart_beat_signaller_source() = channel_source;
  event->Signal();
}

void MasterProxy::RegisterClient() {
  RegisterClientRequest* request = new RegisterClientRequest();
  *request->mutable_client_info() = client_info_;
  RegisterClientResponse* response = new RegisterClientResponse();

#if defined(FEL_WIN_NO_GRPC)
  master_client_interface_->RegisterClientAsync(
      request, response,
      base::BindOnce(&MasterProxy::OnRegisterClient, base::Unretained(this),
                     nullptr, base::Owned(request), base::Owned(response)));
#else
  base::WaitableEvent* event = new base::WaitableEvent;
  master_client_interface_->RegisterClientAsync(
      request, response,
      base::BindOnce(&MasterProxy::OnRegisterClient, base::Unretained(this),
                     event, base::Owned(request), base::Owned(response)));
  event->Wait();
  delete event;
#endif
}

void MasterProxy::OnRegisterClient(base::WaitableEvent* event,
                                   const RegisterClientRequest* request,
                                   RegisterClientResponse* response, Status s) {
  if (s.ok()) {
    client_info_.set_id(response->id());
#if defined(FEL_WIN_NO_GRPC)
    DCHECK(!event);
    is_client_info_set_ = true;
#else
    event->Signal();
#endif
  } else {
    LOG(FATAL) << s;
  }
}

void MasterProxy::OnRegisterNodeAsync(std::unique_ptr<NodeLifecycle> node,
                                      const RegisterNodeRequest* request,
                                      RegisterNodeResponse* response,
                                      Status s) {
  if (!s.ok()) {
    Status new_status(s.error_code(),
                      base::StringPrintf("Failed to register node : %s",
                                         s.error_message().c_str()));
    node->OnError(new_status);
    return;
  }

  std::unique_ptr<NodeInfo> node_info(response->release_node_info());
#if defined(OS_WIN) && NDEBUG
  // FIXME: I don't know why but on windows std::move(*node_info) causes an error.
  node->OnDidCreate(*node_info);
#else
  node->OnDidCreate(std::move(*node_info));
#endif
  nodes_.push_back(std::move(node));
}

void MasterProxy::SubscribeTopicAsync(
    const SubscribeTopicRequest* request, SubscribeTopicResponse* response,
    StatusOnceCallback callback,
    MasterNotificationWatcher::NewTopicInfoCallback topic_info_callback) {
  master_notification_watcher_.RegisterTopicInfoCallback(request->topic(),
                                                         topic_info_callback);
  master_client_interface_->SubscribeTopicAsync(request, response,
                                                std::move(callback));
}

void MasterProxy::RegisterServiceClientAsync(
    const RegisterServiceClientRequest* request,
    RegisterServiceClientResponse* response, StatusOnceCallback callback,
    MasterNotificationWatcher::NewServiceInfoCallback service_info_callback) {
  master_notification_watcher_.RegisterServiceInfoCallback(
      request->service(), service_info_callback);
  master_client_interface_->RegisterServiceClientAsync(request, response,
                                                       std::move(callback));
}

void MasterProxy::UnsubscribeTopicAsync(const UnsubscribeTopicRequest* request,
                                        UnsubscribeTopicResponse* response,
                                        StatusOnceCallback callback) {
  master_notification_watcher_.UnregisterTopicInfoCallback(request->topic());
  master_client_interface_->UnsubscribeTopicAsync(request, response,
                                                  std::move(callback));
}

void MasterProxy::UnregisterServiceClientAsync(
    const UnregisterServiceClientRequest* request,
    UnregisterServiceClientResponse* response, StatusOnceCallback callback) {
  master_notification_watcher_.UnregisterServiceInfoCallback(
      request->service());
  master_client_interface_->UnregisterServiceClientAsync(request, response,
                                                         std::move(callback));
}

}  // namespace felicia