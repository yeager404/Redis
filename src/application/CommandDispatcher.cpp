#include "redis/application/CommandDispatcher.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace redis::application {
  namespace  {
    // Redis command names are conventionally case-insensitive.
    std::string uppercase(std::string value) {
      std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) {
          return static_cast<char>(std::toupper(ch));
        }
      );
      return value;
    }
  }// namespace
   
  CommandDispatcher::CommandDispatcher(storage::IKeyValueStore& store): store_(store){}

  Response CommandDispatcher::execute(const Request& request) {
    if(request.args.empty()){
      return Response::error("ERR empty command");
    }

    const std::string command = uppercase(request.args[0]);

    if(command == "PING"){
      if(request.args.size() == 1){
        return Response::string("PONG");
      }

      if(request.args.size() == 2){
        return Response::string(request.args[1]);
      }

      return Response::error("ERR wrong number of arguments for PING");
    }

    if(command == "SET") {
      if(request.args.size() != 3){
        return Response::error("ERR wrong number of arguments for SET");
      }

      store_.set(request.args[1], request.args[2]);
      return Response::string("OK");
    }

    if(command == "GET"){
      if(request.args.size() != 2){
        return Response::error("ERR wrong number of argumnets for GET");
      }

      auto value = store_.get(request.args[1]);

      if(!value.has_value()){
        return Response::null();
      }

      return Response::string(*value);
    }
    
    if(command == "DEL") {
      if(request.args.size() != 2){
        return Response::error("ERR wrong number of arguments for DEL");
      }

      return Response::integer(store_.del(request.args[1]) ? 1 : 0 );
    }
    
    return Response::error("ERR unknown command");
  }
} // namespace redis::application
