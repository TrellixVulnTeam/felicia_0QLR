#ifndef FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_CAMERA_CC_CAMERA_SUBSCRIBING_NODE_H_
#define FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_CAMERA_CC_CAMERA_SUBSCRIBING_NODE_H_

#include "felicia/core/communication/subscriber.h"
#include "felicia/core/node/node_lifecycle.h"
#include "felicia/drivers/camera/camera_frame_message.pb.h"

namespace felicia {

class CameraSubscribingNode : public NodeLifecycle {
 public:
  CameraSubscribingNode(const std::string& topic) : topic_(topic) {}

  void OnInit() override {
    std::cout << "CameraSubscribingNode::OnInit()" << std::endl;
  }

  void OnDidCreate(const NodeInfo& node_info) override {
    std::cout << "CameraSubscribingNode::OnDidCreate()" << std::endl;
    node_info_ = node_info;
    RequestSubscribe();
  }

  void OnError(const Status& s) override {
    std::cout << "CameraSubscribingNode::OnError()" << std::endl;
    LOG(ERROR) << s.error_message();
  }

  void RequestSubscribe() {
    communication::Settings settings;
    settings.queue_size = 1;
    settings.period = ::base::TimeDelta::FromMilliseconds(100);
    settings.is_dynamic_buffer = true;

    subscriber_.RequestSubscribe(
        node_info_, topic_, ChannelDef::TCP,
        ::base::BindRepeating(&CameraSubscribingNode::OnMessage,
                              ::base::Unretained(this)),
        ::base::BindRepeating(&CameraSubscribingNode::OnSubscriptionError,
                              ::base::Unretained(this)),
        settings,
        ::base::BindOnce(&CameraSubscribingNode::OnRequestSubscribe,
                         ::base::Unretained(this)));
  }

  void OnMessage(CameraFrameMessage&& message) {
    std::cout << "CameraSubscribingNode::OnMessage()" << std::endl;

    // static int frame_number = 0;
    // char filename[15];
    // frame_number++;
    // sprintf(filename, "frame-%d.argb", frame_number);
    // FILE* fp = fopen(filename, "wb");

    // fwrite(message.data().c_str(), message.data().size(), 1, fp);

    // fflush(fp);
    // fclose(fp);
  }

  void OnSubscriptionError(const Status& s) {
    std::cout << "CameraSubscribingNode::OnSubscriptionError()" << std::endl;
    LOG(ERROR) << s.error_message();
  }

  void OnRequestSubscribe(const Status& s) {
    std::cout << "CameraSubscribingNode::OnRequestSubscribe()" << std::endl;
    LOG_IF(ERROR, !s.ok()) << s.error_message();
  }

  void RequestUnsubscribe() {
    subscriber_.RequestUnsubscribe(
        node_info_, topic_,
        ::base::BindOnce(&CameraSubscribingNode::OnRequestUnsubscribe,
                         ::base::Unretained(this)));
  }

  void OnRequestUnsubscribe(const Status& s) {
    std::cout << "CameraSubscribingNode::OnRequestUnsubscribe()" << std::endl;
    LOG_IF(ERROR, !s.ok()) << s.error_message();
  }

 private:
  NodeInfo node_info_;
  std::string topic_;
  Subscriber<CameraFrameMessage> subscriber_;
};

}  // namespace felicia

#endif  // FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_CAMERA_CC_CAMERA_SUBSCRIBING_NODE_H_