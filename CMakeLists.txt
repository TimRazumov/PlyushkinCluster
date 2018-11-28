cmake_minimum_required(VERSION 3.12)
project(plyushkincluster)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.45.0 COMPONENTS filesystem)

include_directories(project/include)

add_executable(MDS
        project/src/MDS/MDS_main.cpp
        CMakeLists.txt)

add_executable(CS
        project/src/CS/CS_main.cpp
        CMakeLists.txt)

target_link_libraries(CS rpc pthread Boost::filesystem)
target_link_libraries(MDS rpc pthread Boost::filesystem)