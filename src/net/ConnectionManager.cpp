#include "redis/net/ConnectionManager.h"
#include "redis/net/Connection.h"
#include <cstddef>
#include <memory>

namespace redis::net {
  Connection& ConnectionManager::add(int fd){
    auto conn = std::make_unique<Connection>(fd);
    Connection& ref = *conn;

    connections_[fd] = std::move(conn);
    return ref;
  }
  
  Connection* ConnectionManager::get(int fd){
    auto it = connections_.find(fd);
    if(it == connections_.end()){
      return nullptr;
    }

    return it->second.get();
  }

  void ConnectionManager::remove(int fd){
    connections_.erase(fd);
  }
}// namespace redis::net
