#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <rpc/this_handler.h>
#include <zk/client.hpp>

#include "utils.hpp"
#include "ChunkServer.h"



int main(int argc, const char *argv[]) {
    uint16_t rpc_server_port;
    std::string mds_ip;
    std::string mds_port;


    if (argc != 2 || !str_to_uint16(argv[1], rpc_server_port)) {
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        return 1;
    }


    ChunkServer chunkServer(rpc_server_port);

    std::cout << "Please, enter the ZooKeeper server ip and port: " << std::endl;

    std::cin >> mds_ip >> mds_port;

    chunkServer.connect_to_zks(mds_ip, mds_port);
    if (!chunkServer.is_connected_to_zks()) {
        std::cout << "Can't connect to ZooKeeper server. Shutting down." << std::endl;
        return 1;
    }

    std::cout << "Process chunk server registration..." << std::endl;
    chunkServer.register_cs_rpc();


    while(!chunkServer.is_registered()) {

        std::string user_answer;

        std::cout << "Error in chunk server registration. Retry? (type 'yes'/'no')" << std::endl;
        std::cin >> user_answer;


        if (user_answer == "yes") {
            chunkServer.register_cs_rpc();
        } else if (user_answer == "no") {
            std::cout << "Registration cancelled. Shutting down." << std::endl;
            return 0;
        } else {
            std::cout << "Please, type 'yes' or 'no'." << std::endl;
        }

        std::cin.clear();
    }

    std::cout << "Registered successfully. Starting RPC server." << std::endl;

    chunkServer.start_rpc_server();

    return 0;
}