#pragma once

#include "IKeyValueStore.h"
#include "redis/storage/IKeyValueStore.h"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace redis::storage {
  class InMemoryKeyValueStore final : public IKeyValueStore {
    public:
      bool set (const std::string& key, const std::string& value) override;
      std::optional<std::string> get(const std::string& key) const override;
      bool del(const std::string& key) override;
    
    private:
      // A mutex to keep the implementation safe, if commands are later
      // dispatched from multiple threads.
      
      mutable std::mutex mutex_;
      std::unordered_map<std::string, std::string> values_;
  };
} // namespace redis::storage
