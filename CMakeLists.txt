cmake_minimum_required(VERSION 3.10)
project(rpc_daemon)

set(CMAKE_CXX_STANDARD 17)

add_executable(rpc_daemon
               project/src/rpc_daemon.cpp)

target_link_libraries(rpc_daemon rpc pthread)
