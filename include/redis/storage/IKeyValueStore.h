#pragma once

#include <optional>
#include <string>

namespace redis::storage {
  class IKeyValueStore {
    public:
      virtual ~IKeyValueStore() = default;
      virtual bool set(const std::string& key, const std::string& value) = 0;
      virtual std::optional<std::string> get(const std::string& key) const = 0;
      virtual bool del(const std::string& key) = 0;
  };
} // namespace redis::storage
