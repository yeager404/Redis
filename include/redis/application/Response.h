#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <utility>

namespace redis::application {
  struct ErrorResponse {
    std::string message;
  };

  struct NullResponse {};

  struct Response {
    using Array = std::vector<Response>;
    using Data = std::variant<
      std::monostate,
      std::string,
      std::int64_t,
      ErrorResponse,
      NullResponse,
      Array
    >;

    Data value;

    static Response string(std::string text) {
      return Response{std::move(text)};
    }

    static Response integer(std::int64_t number) {
      return Response{number};
    }

    static Response error(std::string message){
      return Response{ErrorResponse{std::move(message)}};
    }

    static Response null() {
      return Response{NullResponse{}};
    }

    static Response array(Array values){
      return Response{std::move(values)};
    }
  };

} // namespace redis::application
