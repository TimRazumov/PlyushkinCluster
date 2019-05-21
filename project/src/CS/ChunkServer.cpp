//
// Created by nickeskov on 21.05.19.
//

#include "ChunkServer.h"

ChunkServer::ChunkServer(uint16_t rpc_server_port)
                : m_rpc_server_port(rpc_server_port)
                , m_rpc_server(rpc_server_port)
                , m_zk_client(zk::client())
                , m_cs_dir(
                            std::string(getenv("HOME")) +
                            "/plyushkincluster/servers/ChunkServer/" +
                            std::to_string(rpc_server_port) +
                            "/"
                          ) {
    add_log(m_cs_dir, "The rpc server is running. Port: " + std::to_string(rpc_server_port));
    std::cout << "The rpc server is running. Port: " << rpc_server_port << std::endl;


    if (boost::filesystem::create_directories(m_cs_dir)) {
        std::cout << "Create directory " << m_cs_dir << std::endl;
        add_log(m_cs_dir, "Create directory " + m_cs_dir);
    }

    this->binding(m_cs_dir);
}

bool ChunkServer::connect_to_zks(std::string& ip, std::string& port) {
    auto address = ip + port;
    auto log = "connect_to_zk_server: " + address;

    try {
        m_zk_client = zk::client::connect("zk://" + address).get();

        m_is_connected_to_zks = true;
        log = "#SUCCESS#" + log;
        std::cout << log << std::endl;

    } catch (zk::error& error) {
        m_is_connected_to_zks = false;

        log = "#FAILURE# " + log + " # " + error.what();
        std::cerr << log << std::endl;
    }

    add_log(m_cs_dir, log);

    return m_is_connected_to_zks;
}

void ChunkServer::start_rpc_server() {
    m_rpc_server.run();
}

bool ChunkServer::is_connected_to_zks() {
    return m_is_connected_to_zks;
}

bool ChunkServer::is_registered() {
    return m_is_registered;
}

bool ChunkServer::register_cs_rpc() {
    if (!m_is_connected_to_zks) {
        m_is_registered = false;
        return false;
    }

    if (!m_is_registered){
        return true;
    }

    try {
        auto cs_global_node = m_zk_client.get("/Cluster/CS").get();

        auto cluster_cs_data = ClusterCsData(nlohmann::json::parse(cs_global_node.data().data()));
        auto concrete_cs_data = ConcreteCsData(m_rpc_server_port);

        m_this_cs_path = "/Cluster/CS/" + std::to_string(cluster_cs_data.get_id());
        auto data = concrete_cs_data.get_data().dump();

        m_zk_client.create(m_this_cs_path, zk::buffer(data.begin(), data.end()), zk::create_mode::ephemeral);

        m_is_registered = true;
    } catch (zk::error& error) {
        std::string log = "#FAILURE# register_cs_rpc # ";
        log += error.what();
        std::cerr << log << std::endl;

        return false;
    }

    return true;
}

void ChunkServer::binding(std::string& cs_dir) {
// возвращает все исключения, возникшие в забинденных ф-ях в клиент TODO: включить когда настанет время
    // rpc_server.suppress_exceptions(true);

    m_rpc_server.bind(
            "save_chunk", [=](const std::string &chunk_UUID, const std::vector<char> &chunk_content) {
                add_log(cs_dir, "save_chunk");

                std::ofstream chunk_file(cs_dir + chunk_UUID);

                for (auto const &symbol : chunk_content) {
                    chunk_file << symbol;
                }

                chunk_file.close();
            }
    );

    m_rpc_server.bind(
            "get_chunk", [=](const std::string &chunk_UUID) {
                add_log(cs_dir, "get_chunk");

                std::ifstream chunk_file(cs_dir + chunk_UUID);
                std::vector<char> chunk_content;
                char buffer;

                chunk_file.get(buffer);
                while (!chunk_file.eof()) {
                    chunk_content.push_back(buffer);
                    chunk_file.get(buffer);
                };

                chunk_file.close();
                return chunk_content;
            }
    );

    m_rpc_server.bind(
            "rename_chunk", [=](const std::string &old_UUID, const std::string &new_UUID) {
                add_log(cs_dir, "rename_chunk");

                boost::filesystem::rename(cs_dir + old_UUID, cs_dir + new_UUID);
            }
    );

    m_rpc_server.bind(
            "delete_chunk", [=](const std::string &chunk_UUID) {
                add_log(cs_dir, "delete_chunk");

                boost::filesystem::remove(cs_dir + chunk_UUID);
            }
    );
}
