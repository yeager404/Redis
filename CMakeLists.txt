cmake_minimum_required(VERSION 3.20)
project(redis_server LANGUAGES CXX)

# USE C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()


# ========= Include Directories ===========
include_directories(${PROJECT_SOURCE_DIR}/include)

# ========= RESP2 Protocol Library =============
add_library(redis_protocol
  src/protocol/resp/RespParser.cpp
  src/protocol/resp/RespEncoder.cpp
  src/protocol/resp/Resp2Codec.cpp
  src/protocol/resp/Resp2ProtocolFactory.cpp
)

# ------ Public Headers-----------
target_include_directories(redis_protocol
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# ========= Networking Library =============
add_library(redis_net
  src/net/Buffer.cpp
  src/net/Connection.cpp
  src/net/ConnectionManager.cpp
  src/net/EpollPoller.cpp
  src/net/TcpServer.cpp
  src/net/EventLoop.cpp
)

target_include_directories(redis_net
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# ========= Server Executable =============
add_executable(redis_server
  src/main.cpp
)

target_link_libraries(redis_server
  PRIVATE
    redis_net
    redis_protocol
)

# ============ Storage Library ============
add_library(redis_storage
  src/storage/InMemoryKeyValueStore.cpp
)

target_include_directories(redis_storage PUBLIC include)

# ========= Application Library ===========

add_library(redis_application
  src/application/CommandDispatcher.cpp 
)

target_include_directories(redis_application PUBLIC include)

target_link_libraries(redis_application PUBLIC redis_storage)

add_executable(command_dispatcher_test
  tests/unit/application/command_dispatcher_test.cpp
)

target_link_libraries(
  command_dispatcher_test
  PRIVATE redis_application
)

add_test(
  NAME command_dispatcher_test
  COMMAND command_dispatcher_test
)

# ========= TESTING =========

#Enable CTest
include(CTest)

if(BUILD_TESTING)
  enable_testing()

  # RESP Parser Unit Test
  add_executable(resp_parser_test
    tests/unit/protocol/resp_parser_test.cpp
  )
  target_link_libraries(resp_parser_test
    PRIVATE redis_protocol
  )
  add_test(
    NAME RESPParserTest
    COMMAND resp_parser_test
  )

  # RESP Codec & Encoder Unit Test
  add_executable(resp_codec_test
    tests/unit/protocol/resp_codec_test.cpp
  )
  target_link_libraries(resp_codec_test
    PRIVATE redis_protocol
  )
  add_test(
    NAME RESPCodecTest
    COMMAND resp_codec_test
  )

  # Buffer Unit Test
  add_executable(buffer_test
    tests/unit/net/buffer_test.cpp
  )
  target_link_libraries(buffer_test
    PRIVATE redis_net
  )
  add_test(
    NAME NetBufferTest
    COMMAND buffer_test
  )

  # Connection & ConnectionManager Unit Test
  add_executable(connection_test
    tests/unit/net/connection_test.cpp
  )
  target_link_libraries(connection_test
    PRIVATE redis_net
  )
  add_test(
    NAME NetConnectionTest
    COMMAND connection_test
  )

  # Storage
  add_executable(storage_test
    tests/unit/application/storage_test.cpp
  )

  target_link_libraries(storage_test PRIVATE redis_storage)
  
  add_test(NAME storage_test COMMAND storage_test)

endif()
