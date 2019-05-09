#ifndef FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_CAMERA_CC_CAMERA_PUBLISHING_NODE_H_
#define FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_CAMERA_CC_CAMERA_PUBLISHING_NODE_H_

#include "felicia/core/communication/publisher.h"
#include "felicia/core/node/node_lifecycle.h"
#include "felicia/drivers/camera/camera_factory.h"

namespace felicia {

class CameraPublishingNode : public NodeLifecycle {
 public:
  CameraPublishingNode(const std::string& topic,
                       const std::string& channel_type,
                       const CameraDescriptor& camera_descriptor,
                       size_t buffer_size)
      : topic_(topic),
        camera_descriptor_(camera_descriptor),
        buffer_size_(buffer_size) {
    ChannelDef_Type type;
    ChannelDef_Type_Parse(channel_type, &type);
    channel_def_.set_type(type);
  }

  void OnInit() override {
    std::cout << "CameraPublishingNode::OnInit()" << std::endl;
    camera_ = CameraFactory::NewCamera(camera_descriptor_);
    CHECK(camera_->Init().ok());
  }

  void OnDidCreate(const NodeInfo& node_info) override {
    std::cout << "CameraPublishingNode::OnDidCreate()" << std::endl;
    node_info_ = node_info;
    RequestPublish();
  }

  void OnError(const Status& s) override {
    std::cout << "CameraPublishingNode::OnError()" << std::endl;
    LOG(ERROR) << s.error_message();
  }

  void RequestPublish() {
    communication::Settings settings;
    settings.buffer_size = Bytes::FromBytes(buffer_size_);

    publisher_.RequestPublish(
        node_info_, topic_, channel_def_, settings,
        ::base::BindOnce(&CameraPublishingNode::OnRequestPublish,
                         ::base::Unretained(this)));
  }

  void OnRequestPublish(const Status& s) {
    std::cout << "CameraPublishingNode::OnRequestPublish()" << std::endl;
    if (s.ok()) {
      MasterProxy& master_proxy = MasterProxy::GetInstance();
      master_proxy.PostTask(FROM_HERE,
                            ::base::BindOnce(&CameraPublishingNode::StartCamera,
                                             ::base::Unretained(this)));
    } else {
      LOG(ERROR) << s.error_message();
    }
  }

  void StartCamera() {
    // You should set the camera format if you have any you want to run with.
    Status s = camera_->Start(
        CameraFormat(640, 480, PIXEL_FORMAT_YUY2, 25),
        ::base::BindRepeating(&CameraPublishingNode::OnCameraFrame,
                              ::base::Unretained(this)),
        ::base::BindRepeating(&CameraPublishingNode::OnCameraError,
                              ::base::Unretained(this)));
    if (s.ok()) {
      // MasterProxy& master_proxy = MasterProxy::GetInstance();
      // master_proxy.PostDelayedTask(
      //     FROM_HERE,
      //     ::base::BindOnce(&CameraPublishingNode::RequestUnpublish,
      //                      ::base::Unretained(this)),
      //     ::base::TimeDelta::FromSeconds(10));
    } else {
      LOG(ERROR) << s.error_message();
    }
  }

  void OnCameraFrame(CameraFrame camera_frame) {
    std::cout << "CameraPublishingNode::OnCameraFrame" << std::endl;
    if (publisher_.IsUnregistered()) return;

    publisher_.Publish(camera_frame.ToCameraFrameMessage(),
                       ::base::BindOnce(&CameraPublishingNode::OnPublish,
                                        ::base::Unretained(this)));
  }

  void OnCameraError(const Status& s) {
    std::cout << "CameraPublishingNode::OnCameraError" << std::endl;
    LOG(ERROR) << s.error_message();
  }

  void OnPublish(const Status& s) {
    std::cout << "CameraPublishingNode::OnPublish()" << std::endl;
    LOG_IF(ERROR, !s.ok()) << s.error_message();
  }

  void RequestUnpublish() {
    publisher_.RequestUnpublish(
        node_info_, topic_,
        ::base::BindOnce(&CameraPublishingNode::OnRequestUnpublish,
                         ::base::Unretained(this)));
  }

  void OnRequestUnpublish(const Status& s) {
    std::cout << "CameraPublishingNode::OnRequestUnpublish()" << std::endl;
    if (s.ok()) {
      MasterProxy& master_proxy = MasterProxy::GetInstance();
      master_proxy.PostTask(FROM_HERE,
                            ::base::BindOnce(&CameraPublishingNode::StopCamera,
                                             ::base::Unretained(this)));
    } else {
      LOG(ERROR) << s.error_message();
    }
  }

  void StopCamera() {
    Status s = camera_->Stop();
    LOG_IF(ERROR, !s.ok()) << s.error_message();
  }

 private:
  NodeInfo node_info_;
  std::string topic_;
  ChannelDef channel_def_;
  CameraDescriptor camera_descriptor_;
  size_t buffer_size_;
  Publisher<CameraFrameMessage> publisher_;
  std::unique_ptr<CameraInterface> camera_;
};

}  // namespace felicia

#endif  // FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_CAMERA_CC_CAMERA_PUBLISHING_NODE_H_