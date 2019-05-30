#include <iostream>
#include <fstream>
#include <string>

#include "utils.hpp"
#include "ChunkServer.h"



int main(int argc, const char *argv[]) {
    uint16_t rpc_server_port = 0;
    std::string mds_ip;
    std::string mds_port;


    if (argc != 2 || !str_to_uint16(argv[1], rpc_server_port)) {
        rpc_server_port = 0;
    }
    std::cout << "Reading configuration from 'cs_zoo.cfg'" << std::endl;

    CsConfig cs_config = ChunkServer::read_cs_config("cs_zoo.cfg");

    if (rpc_server_port != 0) {
        cs_config.rpc_server_port = rpc_server_port;
    }

    ChunkServer chunk_server(cs_config);

    chunk_server.connect_to_zks();
    if (!chunk_server.is_connected_to_zks()) {
        std::cout << "Can't connect to ZooKeeper server. Shutting down." << std::endl;
        return 1;
    }

    std::cout << "Process chunk server registration..." << std::endl;
    chunk_server.register_cs_rpc();


    while(!chunk_server.is_registered()) {

        std::string user_answer;

        std::cout << "Error in chunk server registration. Retry? (type 'yes'/'no')" << std::endl;
        std::cin >> user_answer;


        if (user_answer == "yes") {
            cs_config = ChunkServer::read_cs_config("cs_zoo.cfg");
            chunk_server = ChunkServer(cs_config);
            chunk_server.register_cs_rpc();
        } else if (user_answer == "no") {
            std::cout << "Registration cancelled. Shutting down." << std::endl;
            return 0;
        } else {
            std::cout << "Please, type 'yes' or 'no'." << std::endl;
        }

        std::cin.clear();
    }

    std::cout << "Registered successfully. Starting RPC server." << std::endl;

    chunk_server.start_rpc_server();

    return 0;
}