cmake_minimum_required(VERSION 3.10)
project(plyushkincluster)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall --pedantic -g")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(FUSE REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(project/include
        project/include/libfuse
        project/include/MDS
        project/include/CS
        project/include/Client)


if(FUSE_FOUND)
    add_definitions(${FUSE_DEFINITIONS})
    include_directories(project/include/libfuse ${FUSE_INCLUDE_DIRS})
    add_executable(fuse_mount.out
            project/src/Client/Fuse.cpp
            project/src/Client/mounter_main.cpp
            project/src/Client/mounter.cpp
            project/src/utils.cpp)

    target_link_libraries(fuse_mount.out rpc pthread Boost::filesystem ${FUSE_LIBRARIES})
endif()

add_executable(connect.out
        project/src/Client/connector.cpp
        project/src/utils.cpp)

target_link_libraries(connect.out rpc pthread)

add_executable(MDS.out
        project/src/MDS/MDS_main.cpp
        project/src/MDS/MDS.cpp
        project/src/utils.cpp project/src/EntitiesInfo/MDS_data.cpp project/src/EntitiesInfo/CS_data.cpp
        project/src/MDS/CS_Watcher.cpp)

target_link_libraries(MDS.out rpc pthread Boost::filesystem nlohmann_json::nlohmann_json)

add_executable(CS.out
        project/src/CS/CS_main.cpp
        project/src/utils.cpp project/src/CS/ChunkServer.cpp
        project/src/EntitiesInfo/MDS_data.cpp project/src/EntitiesInfo/CS_data.cpp)


target_link_libraries(CS.out rpc pthread Boost::filesystem nlohmann_json::nlohmann_json)
link_directories(/usr/local/lib)
target_link_libraries(MDS.out zkpp zkpp-server)
target_link_libraries(CS.out zkpp)