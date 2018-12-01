cmake_minimum_required(VERSION 3.10)
project(fuse_mount)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall --pedantic -g")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

find_package(FUSE REQUIRED)
if(FUSE_FOUND)
add_definitions(${FUSE_DEFINITIONS})
include_directories(project/include ${FUSE_INCLUDE_DIRS})
add_executable(fuse_mount
                project/src/Fuse.cpp
                project/src/hello.cpp
                project/src/helloFS.cpp)

target_link_libraries(fuse_mount ${FUSE_LIBRARIES})
endif()
