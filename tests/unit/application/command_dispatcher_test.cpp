#include "redis/storage/InMemoryKeyValueStore.h"
#include "redis/application/CommandDispatcher.h"

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <variant>

using redis::application::CommandDispatcher;
using redis::application::Request;
using redis::application::Response;

int main() {
  redis::storage::InMemoryKeyValueStore store;
  redis::application::CommandDispatcher dispatcher(store);

  // PING
  {
    Response response = dispatcher.execute({{"PING"}});
    auto* text = std::get_if<std::string>(&response.value);

    assert(text != nullptr);
    assert(*text == "PONG");
  }

  // SET then GET
  {
    Response setResponse = dispatcher.execute({{"SET", "color", "blue"}});

    auto* setText = std::get_if<std::string>(&setResponse.value);
    assert(setText != nullptr);
    assert(*setText == "OK");

    Response getResponse = dispatcher.execute({{"GET", "color"}});

    auto* getText = std::get_if<std::string>(&getResponse.value);
    assert(getText != nullptr);
    assert(*getText == "blue");
  }

  // DEL
  {
    Response response = dispatcher.execute({{"DEL", "color"}});
    auto* number = std::get_if<std::int64_t>(&response.value);
    assert(number != nullptr);
    assert(*number == 1);
  }

  // Unknown Command 
  {
    Response response = dispatcher.execute({{"SOMETHING"}});

    assert(std::holds_alternative<redis::application::ErrorResponse>(response.value));
  }

  std::cout << "Command dispatcher tests passed \n";
}
