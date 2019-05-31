#ifndef PLYUSHKINCLUSTER_MDS_H
#define PLYUSHKINCLUSTER_MDS_H


#include <vector>
#include <rpc/server.h>
#include <rpc/client.h>
 
#include <zk/client.hpp>
#include <zk/multi.hpp>
#include <zk/server/configuration.hpp>
#include <zk/server/server.hpp>
#include <zk/server/server_group.hpp>
#include <zk/buffer.hpp>
#include <nlohmann/json.hpp>

#include "EntitiesInfo/MDS_data.h"
#include "EntitiesInfo/CS_data.h"


// TODO: проверка, не отвалились ли ChunkServer
// TODO: действия в случае отключения сервака
// TODO: организация нескольких MDS
// TODO: daemon
// TODO: sysadmin tools: create and add server, set password, safety stop server, etc


struct MdsConfig{
    uint16_t rpc_server_port;
    uint16_t zks_port;
    std::string zks_ip;
};


// основной класс для демона, работающего на серверах метаданных
class MDS {
private:
    // конфига
    MdsConfig m_config;

    std::string MDS_directory;

    // я хз, шо тут комментировать. Крч, основа класса. Не стал делать наследником по причине "ну нахер"
    rpc::server this_server;

    // поддержание связи с другими серваками
    std::vector<CS_data> known_CS;

    // information
    size_t cs_timeout;

    // количество копий чанка на серверах
    std::size_t copy_count;

    // номер сервера в , на который будет класться следующий чанк
    std::size_t turn_of_servers;

    // бинд ф-й, которые может выполнять сервер по запросу клиента
    void binding();
    
    // for zk
    void create_empty_node(const std::string &dir, zk::create_mode node_type);
    bool exists_node(const std::string &path);
    bool is_access();
    zk::client my_zk_clt;

public:

    explicit MDS(const MdsConfig &config);

    static MdsConfig read_mds_config(const char *config_name);
    static MdsConfig read_mds_config(std::string &config_name);

    ~MDS() = default;

    void async_run(const std::size_t &worker_threads = 1) {
        this_server.async_run(worker_threads);
    }

    void stop() {
        this_server.stop();
    }

    std::vector<CS_data> const get_known_CS() {
        return known_CS;
    }

    int64_t const get_cs_timeout() {
        return cs_timeout;
    }

    zk::client get_zk_client() {
        return my_zk_clt;
    }


    void add_CS(const std::string &addr, uint16_t port);

    void set_cs_timeout(int64_t new_timeout);
};


#endif //PLYUSHKINCLUSTER_MDS_H
