//
// Created by nickeskov on 21.05.19.
//

#ifndef PLYUSHKINCLUSTER_CHUNKSERVER_H
#define PLYUSHKINCLUSTER_CHUNKSERVER_H


#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <rpc/server.h>
#include <rpc/this_handler.h>

#include <zk/client.hpp>
#include <zk/error.hpp>

#include <nlohmann/json.hpp>

#include "utils.hpp"
#include "EntitiesInfo/CS_data.h"
#include "CS_data.h"


#define MEGABYTE 1000000

struct CsConfig{
    uint16_t rpc_server_port;
    uint16_t zks_port;
    std::string zks_ip;
};


class ChunkServer {
public:
    explicit ChunkServer(CsConfig& config);

    static CsConfig read_cs_config(const char *config_name);
    static CsConfig read_cs_config(std::string &config_name);

    bool connect_to_zks();

    bool connect_to_zks(std::string& ip, std::string& port);

    void start_rpc_server();

    bool register_cs_rpc();

    bool is_connected_to_zks();

    bool is_registered();


private:
    CsConfig m_config;
    rpc::server m_rpc_server;
    std::unique_ptr<zk::client> m_zk_client;

    std::string m_cs_dir;
    std::string m_this_cs_path;
    bool m_is_connected_to_zks = false;
    bool m_is_registered = false;

    void binding(std::string& cs_dir);

};


#endif //PLYUSHKINCLUSTER_CHUNKSERVER_H
