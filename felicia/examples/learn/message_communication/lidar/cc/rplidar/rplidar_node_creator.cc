#include "felicia/core/felicia_init.h"
#include "felicia/core/master/master_proxy.h"
#include "felicia/examples/learn/message_communication/lidar/cc/lidar_subscribing_node.h"
#include "felicia/examples/learn/message_communication/lidar/cc/rplidar/rplidar_flag.h"
#include "felicia/examples/learn/message_communication/lidar/cc/rplidar/rplidar_publishing_node.h"

namespace felicia {

int RealMain(int argc, char* argv[]) {
  FeliciaInit();

  RPLidarFlag delegate;
  FlagParser parser;
  parser.set_program_name("rplidar_node_creator");
  if (!parser.Parse(argc, argv, &delegate)) {
    return 1;
  }

  MasterProxy& master_proxy = MasterProxy::GetInstance();
  Status s = master_proxy.Start();
  if (!s.ok()) {
    std::cerr << kRedError << s << std::endl;
    return 1;
  }

  NodeInfo node_info;
  node_info.set_name(delegate.name_flag()->value());

  if (delegate.is_publishing_node_flag()->value()) {
    drivers::LidarEndpoint lidar_endpoint;
    if (delegate.serial_port_flag()->is_set()) {
      lidar_endpoint =
          drivers::LidarEndpoint(delegate.serial_port_flag()->value(),
                                 delegate.baudrate_flag()->value());
    } else {
      net::IPAddress address;
      bool valid = address.AssignFromIPLiteral(delegate.ip_flag()->value());
      if (!valid) {
        std::cerr << "IP is not valid " << delegate.ip_flag()->value()
                  << std::endl;
      }
      lidar_endpoint = drivers::LidarEndpoint(
          net::IPEndPoint(address, delegate.tcp_port_flag()->value()));
    }
    master_proxy.RequestRegisterNode<RPlidarPublishingNode>(node_info, delegate,
                                                            lidar_endpoint);
  } else {
    master_proxy.RequestRegisterNode<LidarSubscribingNode>(node_info, delegate);
  }

  master_proxy.Run();

  return 0;
}

}  // namespace felicia

int main(int argc, char* argv[]) { return felicia::RealMain(argc, argv); }
