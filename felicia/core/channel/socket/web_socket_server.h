#ifndef FELICIA_CORE_CHANNEL_SOCKET_WEB_SOCKET_SERVER_H_
#define FELICIA_CORE_CHANNEL_SOCKET_WEB_SOCKET_SERVER_H_

#include "felicia/core/channel/socket/tcp_server_socket.h"
#include "felicia/core/channel/socket/web_socket.h"

namespace felicia {

class EXPORT WebSocketServer : public WebSocket {
 public:
  WebSocketServer();
  ~WebSocketServer();

  // Socket methods
  bool IsServer() const override;

  StatusOr<ChannelDef> Listen();

  void AcceptLoop(TCPServerSocket::AcceptCallback callback);

  // ChannelImpl methods
  void Write(char* buffer, int size, StatusOnceCallback callback) override;
  void Read(char* buffer, int size, StatusOnceCallback callback) override;

  // WebSocket methods
  void OnHandshaked(
      StatusOr<std::unique_ptr<::net::TCPSocket>> status_or) override;

 private:
  void DoAcceptOnce();
  void OnAccept(StatusOr<std::unique_ptr<::net::TCPSocket>> status_or);

  TCPServerSocket::AcceptCallback accept_callback_;
  std::unique_ptr<TCPServerSocket> tcp_server_socket_;

  WebSocket::HandshakeHandler handshake_handler_;
};

}  // namespace felicia

#endif  // FELICIA_CORE_CHANNEL_SOCKET_WEB_SOCKET_SERVER_H_