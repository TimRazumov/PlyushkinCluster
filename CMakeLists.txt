cmake_minimum_required(VERSION 3.10)
project(plyushkincluster)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall --pedantic -g")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(FUSE REQUIRED)

include_directories(project/include
                    project/include/libfuse
                    project/include/MDS
                    project/include/CS
                    project/include/Client)


if(FUSE_FOUND)
    add_definitions(${FUSE_DEFINITIONS})
    include_directories(project/include/libfuse ${FUSE_INCLUDE_DIRS})
    add_executable(fuse_mount
            project/src/Client/Fuse.cpp
            project/src/Client/mounter_main.cpp
            project/src/Client/mounter.cpp
            project/src/utils.cpp)

    target_link_libraries(fuse_mount rpc pthread Boost::filesystem ${FUSE_LIBRARIES})
endif()

add_executable(connect
        project/src/Client/connector.cpp
        project/src/utils.cpp)

target_link_libraries(connect rpc pthread)

add_executable(MDS
        project/src/MDS/MDS_main.cpp
        project/src/MDS/MDS.cpp
        project/src/utils.cpp)

target_link_libraries(MDS rpc pthread Boost::filesystem)

add_executable(CS
        project/src/CS/CS_main.cpp
        project/src/utils.cpp)

target_link_libraries(CS rpc pthread Boost::filesystem)
link_directories(/usr/local/lib)
target_link_libraries(MDS zkpp zkpp-server)
