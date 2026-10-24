cmake_minimum_required(VERSION 3.26)

project(redis_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

target_link_libraries(redis_server PRIVATE pthread)
