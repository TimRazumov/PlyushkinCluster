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
void CS_Watcher::run() {
    while (true) {
        auto children = client.get_children("/CLUSTER/CS").get();
        auto real_CS_list = children.children();

        for (const std::string& cs: real_CS_list) {
            std::string real_ip = get_ip(client.get(cs).get());

            try {
                if (known_CS.at(cs) != real_ip) {
                    std::cout << "[CS_Watcher]: " << cs << " changed ip" << std::endl;
                    known_CS.at(cs) = real_ip;
                    // magic
                }
            } catch (const std::out_of_range& ex) {
                std::cout << "[CS_Watcher]: new  znode = " << cs << " | ip = "
                    << real_ip << std::endl;
            }
        }

        if (true) break;
    }
}
