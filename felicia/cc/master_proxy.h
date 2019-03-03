#ifndef FELICIA_CC_MASTER_PROXY_H_
#define FELICIA_CC_MASTER_PROXY_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "third_party/chromium/base/bind.h"
#include "third_party/chromium/base/callback.h"
#include "third_party/chromium/base/containers/flat_map.h"
#include "third_party/chromium/base/macros.h"
#include "third_party/chromium/base/memory/scoped_refptr.h"
#include "third_party/chromium/base/message_loop/message_loop.h"
#include "third_party/chromium/base/no_destructor.h"
#include "third_party/chromium/base/run_loop.h"
#include "third_party/chromium/base/synchronization/waitable_event.h"

#include "felicia/core/channel/channel.h"
#include "felicia/core/lib/base/export.h"
#include "felicia/core/master/heart_beat_signaller.h"
#include "felicia/core/master/master_client_interface.h"
#include "felicia/core/master/topic_info_watcher.h"
#include "felicia/core/node/node_lifecycle.h"

namespace felicia {

class EXPORT MasterProxy final : public TaskRunnerInterface {
 public:
  static MasterProxy& GetInstance();

  void Init();
  void Run();

  // TaskRunnerInterface methods
  bool PostTask(const ::base::Location& from_here,
                ::base::OnceClosure callback) override {
    return message_loop_->task_runner()->PostTask(from_here,
                                                  std::move(callback));
  }

  bool PostDelayedTask(const ::base::Location& from_here,
                       ::base::OnceClosure callback,
                       ::base::TimeDelta delay) override {
    return message_loop_->task_runner()->PostDelayedTask(
        from_here, std::move(callback), delay);
  }

  template <typename NodeTy>
  std::enable_if_t<std::is_base_of<NodeLifecycle, NodeTy>::value, void>
  RequestRegisterNode(const NodeInfo& node_info);

  void PublishTopicAsync(PublishTopicRequest* request,
                         PublishTopicResponse* response,
                         StatusCallback callback);
  void SubscribeTopicAsync(SubscribeTopicRequest* request,
                           SubscribeTopicResponse* response,
                           StatusCallback callback,
                           TopicInfoWatcher::NewTopicInfoCallback callback2);

 private:
  friend class ::base::NoDestructor<MasterProxy>;
  MasterProxy();

  void RegisterClient();

  void OnRegisterClient(::base::WaitableEvent* event,
                        RegisterClientRequest* request,
                        RegisterClientResponse* response, const Status& s);

  template <typename NodeTy>
  void OnRegisterNodeAsync(RegisterNodeRequest* request,
                           RegisterNodeResponse* response, const Status& s);

  std::unique_ptr<::base::MessageLoop> message_loop_;
  std::unique_ptr<::base::RunLoop> run_loop_;
  std::unique_ptr<MasterClientInterface> master_client_interface_;

  ClientInfo client_info_;

  TopicInfoWatcher topic_info_watcher_;
  HeartBeatSignaller heart_beat_signaller_;

  std::vector<std::unique_ptr<NodeLifecycle>> nodes_;

  DISALLOW_COPY_AND_ASSIGN(MasterProxy);
};

template <typename NodeTy>
std::enable_if_t<std::is_base_of<NodeLifecycle, NodeTy>::value, void>
MasterProxy::RequestRegisterNode(const NodeInfo& node_info) {
  RegisterNodeRequest* request = new RegisterNodeRequest();
  NodeInfo* new_node_info = request->mutable_node_info();
  new_node_info->set_client_id(client_info_.id());
  new_node_info->set_name(node_info.name());
  RegisterNodeResponse* response = new RegisterNodeResponse();

  master_client_interface_->RegisterNodeAsync(
      request, response,
      ::base::BindOnce(&MasterProxy::OnRegisterNodeAsync<NodeTy>,
                       ::base::Unretained(this), ::base::Owned(request),
                       ::base::Owned(response)));
}

template <typename NodeTy>
void MasterProxy::OnRegisterNodeAsync(RegisterNodeRequest* request,
                                      RegisterNodeResponse* response,
                                      const Status& s) {
  if (!s.ok()) {
    LOG(ERROR) << "Failed to create node";
    return;
  }

  const NodeInfo& node_info = response->node_info();
  std::unique_ptr<NodeLifecycle> node = std::make_unique<NodeTy>(node_info);
  nodes_.push_back(std::move(node));
  NodeLifecycle* n = nodes_.back().get();
  n->OnInit();
  n->OnDidCreate();
}

}  // namespace felicia

#endif  // FELICIA_CC_MASTER_PROXY_H_