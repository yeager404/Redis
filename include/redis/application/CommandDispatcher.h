#pragma once

#include "Request.h"
#include "Response.h"
#include "redis/storage/IKeyValueStore.h"

namespace redis::application {
  class CommandDispatcher{
    public: 
      explicit CommandDispatcher(storage::IKeyValueStore& store);
      Response execute(const Request& request);

    private:
      storage::IKeyValueStore& store_;
  };
} // namespace redis::application
