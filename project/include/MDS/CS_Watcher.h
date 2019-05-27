//
// Created by artem on 22.05.19.
//

#ifndef PLYUSHKINCLUSTER_CS_WATCHER_H
#define PLYUSHKINCLUSTER_CS_WATCHER_H

#include <zk/client.hpp>
#include <zk/results.hpp>
#include <map>
#include <set>
#include <vector>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unistd.h>
#include <rpc/client.h>

using json = nlohmann::json;

const std::string CS_direcrory = "/CLUSTER/CS";
const std::string meta_directory = "/CLUSTER/META";
const std::string renovation_directory = "/CLUSTER/RENOVATION";
const uint16_t timeout_rpc_connect = 2;

class CS_Watcher final {
public:
    explicit CS_Watcher();
    void run();
    void renovate(const std::set<std::string>& fallen_cs);

    static std::string get_ip(const zk::get_result& res);
    static uint16_t get_port(const zk::get_result& res);
private:
    zk::client client;
    std::map<std::string, std::string> known_CS; // "Znode_name" : "ip:port"
};


class Renovator {
public:
    explicit Renovator(zk::client& client, const std::string& CS_id);
    void run();

private:
    zk::client& client;
    const std::string m_CS_id;

    void restore_raid_0(const std::string& file_node_name);
    void restore_raid_1(const std::string& file_node_name);
    std::vector<char> get_chunk_content(const std::set<std::string>& CS_id_set, const std::string& UUID,
                                        const std::string& chunk_num) const;
    std::string send_chunk(const std::set<std::string>& CS_id_set, const std::string& chunk_UUID,
                    const std::vector<char>& chunk_content);
};



/*
class Renovator_0 : public Renovator {
public:
    explicit Renovator_0(zk::client& client, const std::string& meta_node_name,
            const std::set<std::string>& fallen_cs);
    bool run() override;
};

class Renovator_1 : public Renovator {
public:
    explicit Renovator_1(zk::client& client, const std::string& meta_node_name,
            const std::set<std::string>& fallen_cs);
    bool run() override;

};
*/
#endif //PLYUSHKINCLUSTER_CS_WATCHER_H
