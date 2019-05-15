#include "felicia/core/node/dynamic_publishing_node.h"

namespace felicia {

DynamicPublishingNode::DynamicPublishingNode(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {}

DynamicPublishingNode::~DynamicPublishingNode() = default;

void DynamicPublishingNode::OnDidCreate(const NodeInfo& node_info) {
  node_info_ = node_info;
  delegate_->OnDidCreate(this);
}

void DynamicPublishingNode::OnError(const Status& s) {
  LOG_IF(ERROR, !s.ok()) << s.error_message();
}

void DynamicPublishingNode::RequestPublish(
    const std::string& message_type, const std::string& topic,
    const ChannelDef& channel_def, const communication::Settings& settings) {
  publisher_ = std::make_unique<DynamicPublisher>();
  publisher_->ResetMessage(message_type);

  publisher_->RequestPublish(
      node_info_, topic, channel_def, settings,
      ::base::BindOnce(&DynamicPublishingNode::OnRequestPublish,
                       ::base::Unretained(this)));
}

void DynamicPublishingNode::RequestUnpublish(const std::string& topic) {
  publisher_->RequestUnpublish(
      node_info_, topic,
      ::base::BindOnce(&DynamicPublishingNode::OnRequestUnpublish,
                       ::base::Unretained(this)));
}

void DynamicPublishingNode::PublishMessageFromJson(
    const std::string& json_message) {
  publisher_->PublishFromJson(
      json_message,
      ::base::BindOnce(&DynamicPublishingNode::Delegate::OnPublish,
                       ::base::Unretained(delegate_.get())));
}

void DynamicPublishingNode::OnRequestPublish(const Status& s) {
  if (!s.ok()) {
    publisher_.reset();
  }
  delegate_->OnRequestPublish(s);
}

void DynamicPublishingNode::OnRequestUnpublish(const Status& s) {
  if (s.ok()) {
    publisher_.reset();
  }
  delegate_->OnRequestUnpublish(s);
}

}  // namespace felicia