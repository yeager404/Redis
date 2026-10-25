cmake_minimum_required(VERSION 3.26)

project(redis_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Phase 2: RESP2 protocol library
add_library(redis_protocol
  src/protocol/RespParser.cpp
  src/protocol/RespEncoder.cpp
)

target_include_directories(redis_protocol
  PUBLIC $(PROJECT_SOURCE_DIR)/include
)

target_compile_options(redis_protocol PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

# Phase 1: TCP server
add_executable(redis_server
  src/main.cpp
  src/net/Buffer.cpp
  src/net/Connection.cpp
  src/net/ConnectionManager.cpp
  src/net/EpollPoller.cpp
  src/net/TcpServer.cpp
  src/net/EventLoop.cpp
)

target_include_directories(redis_server
  PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(redis_server PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(redis_server PRIVATE redis_protocol)

# Protocol Tests
include(CTest)

if(BUILD_TESTING)
  add_executable(protocol_test
    tests/protocol_test.cpp
  )

  target_link_libraries(protocol_test
    PRIVATE redis_protocol
  )

  target_compile_options(protocol_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  
  add_test(
    NAME RESP2ProtocolTests
    COMMAND protocol_test
  )
endif()
