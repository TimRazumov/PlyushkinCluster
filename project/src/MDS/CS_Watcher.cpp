//
// Created by artem on 22.05.19.
//

#include "CS_Watcher.h"

// constructor - make connection
CS_Watcher::CS_Watcher() : client("zk//:127.0.0.1:2181") {}

// get ip of cs by zk::get_result
std::string CS_Watcher::get_ip(const zk::get_result& res) const {
    json data = json::parse(res.data());
    return data["ip"].get<std::string>();
}

// run watching
void CS_Watcher::run() const {
    while (true) {
        auto children = client.get_children("/CLUSTER/CS").get();
        auto real_CS_list = children.children();

        for (const auto& cs: real_CS_list) {
            std::string real_ip = client.get(cs)
            try {

            } catch (const std::out_of_range& ex) {

            }
        }

        if (true) break;
    }
}
