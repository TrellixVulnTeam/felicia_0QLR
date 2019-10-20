#if defined(HAS_ROS)

#ifndef FELICIA_CORE_CHANNEL_ROS_SERVICE_REQUEST_H_
#define FELICIA_CORE_CHANNEL_ROS_SERVICE_REQUEST_H_

#include "felicia/core/channel/message_receiver.h"
#include "felicia/core/channel/message_sender.h"
#include "felicia/core/lib/base/export.h"
#include "felicia/core/message/ros_header.h"
#include "felicia/core/message/ros_protocol.h"

namespace felicia {

namespace rpc {

template <typename T, typename SFINAE>
class Client;

}  // namespace rpc

class EXPORT RosServiceRequest {
 public:
  RosServiceRequest();
  ~RosServiceRequest();

  void Reset();

  template <typename T, typename SFINAE>
  void Request(rpc::Client<T, SFINAE>* client, StatusOnceCallback callback) {
    channel_ = client->channel_.get();
    callback_ = std::move(callback);
    RosServiceRequestHeader header;
    ConsumeRosProtocol(client->service_info_.service(), &header.service);
    header.md5sum = client->GetServiceMD5Sum();
    header.callerid = client->service_info_.ros_node_name();
    header.persistent = "1";
    channel_->SetDynamicSendBuffer(true);
    MessageSender<RosServiceRequestHeader> sender(channel_);
    sender.SendMessage(header, base::BindOnce(&RosServiceRequest::OnRequest,
                                              base::Unretained(this)));

    channel_->SetDynamicReceiveBuffer(true);
    receiver_.set_channel(channel_);
    receiver_.ReceiveMessage(base::BindOnce(
        &RosServiceRequest::OnReceiveResponse, base::Unretained(this), header));
  }

  void OnRequest(const Status& s);

  void OnReceiveResponse(const RosServiceRequestHeader& header,
                         const Status& s);

 private:
  Channel* channel_;
  Header header_;
  MessageReceiver<RosServiceResponseHeader> receiver_;
  StatusOnceCallback callback_;
};

}  // namespace felicia

#endif  // FELICIA_CORE_CHANNEL_ROS_SERVICE_REQUEST_H_

#endif  // defined(HAS_ROS)