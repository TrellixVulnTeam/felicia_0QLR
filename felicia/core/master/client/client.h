#ifndef FELICIA_CORE_MASTER_CLIENT_CLIENT_H_
#define FELICIA_CORE_MASTER_CLIENT_CLIENT_H_

#include <memory>
#include <vector>

#include "third_party/chromium/base/macros.h"
#include "third_party/chromium/base/memory/weak_ptr.h"
#include "third_party/chromium/base/threading/thread_collision_warner.h"

#include "felicia/core/lib/base/export.h"
#include "felicia/core/node/node.h"

namespace felicia {

class EXPORT Client {
 public:
  static std::unique_ptr<Client> NewClient(const ClientInfo& client_info);

  ~Client();

  const ClientInfo& client_info() const { return client_info_; }

  // Add the Node.
  void AddNode(std::unique_ptr<Node> node);
  // Remove the Node whose |node_info| is same with a given |node_info|.
  void RemoveNode(const NodeInfo& node_info);
  // Check there has a Node whose |node_info| is same with a given |node_info|.
  bool HasNode(const NodeInfo& node_info) const;
  // Find the node whose |node_info| is same with a given |node_info|.
  ::base::WeakPtr<Node> FindNode(const NodeInfo& node_info);
  // Find the nodes which meets the given condition |node_filter|.
  std::vector<::base::WeakPtr<Node>> FindNodes(const NodeFilter& node_filter);

 private:
  explicit Client(const ClientInfo& client_info) : client_info_(client_info) {}

  ClientInfo client_info_;
  std::vector<std::unique_ptr<Node>> nodes_;
  DFAKE_MUTEX(add_remove_);

  DISALLOW_COPY_AND_ASSIGN(Client);
};

}  // namespace felicia

#endif  // FELICIA_CORE_MASTER_CLIENT_CLIENT_H_