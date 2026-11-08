#include "redis/storage/InMemoryKeyValueStore.h"
#include <mutex>
#include <optional>

namespace redis::storage {
  bool InMemoryKeyValueStore::set(const std::string& key, const std::string& value){
    std::lock_guard lock(mutex_);
    // Insert a new key or replace existing value

    values_[key] = value;
    return true;
  }

  std::optional<std::string>InMemoryKeyValueStore::get(const std::string& key) const {
    std::lock_guard lock(mutex_);
    auto it = values_.find(key);

    if(it == values_.end()) {
      return std::nullopt;
    }

    return it->second;
  }

  bool InMemoryKeyValueStore::del(const std::string&key) {
    std::lock_guard lock(mutex_);

    // erase() returns the number of removed elements.
    return values_.erase(key) > 0;
  }
} // namespace redis::storage
