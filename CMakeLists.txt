cmake_minimum_required(VERSION 3.10)
project(rpc_daemon)

set(CMAKE_CXX_STANDARD 17)

include_directories(project/include)

add_executable(daemon
               project/src/rpc_daemon.cpp project/src/utils.cpp)

target_link_libraries(daemon rpc pthread)
