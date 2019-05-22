//
// Created by artem on 22.05.19.
//

#ifndef PLYUSHKINCLUSTER_CS_WATCHER_H
#define PLYUSHKINCLUSTER_CS_WATCHER_H

#include <zk/client.hpp>
#include <map>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>
#include "CS_data.h"

using json = nlohmann::json;

class CS_Watcher final {
public:
    explicit CS_Watcher();

    void run() const;

private:
    zk::client client;
    std::map<std::string, std::string> known_CS; // Znode_name : ip

    std::string get_ip(const zk::get_result& res) const;
};


#endif //PLYUSHKINCLUSTER_CS_WATCHER_H
