#pragma once
#include <string>
#include <vector>

namespace redis::application {

  // A command and its parameters
  // eg: {"SET", "name", "Alice"}

  struct Request {
    std::vector<std::string> args;
  };
} // namespace redis::application

