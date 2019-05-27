//
// Created by artem on 22.05.19.
//

#include <zk/error.hpp>
#include "CS_Watcher.h"

// constructor - make connection
CS_Watcher::CS_Watcher() : client("zk://127.0.0.1:2181") {}

// get ip of cs by zk::get_result
std::string CS_Watcher::get_ip(const zk::get_result& res) {
    json data = json::parse(res.data());
    return data["ip"].get<std::string>();
}

uint16_t CS_Watcher::get_port(const zk::get_result& res) {
    json data = json::parse(res.data());
    return data["port"].get<uint16_t>();
}

// run watching
void CS_Watcher::run() {
    std::cout << "[CS_Watcher]: STARTED" << std::endl;

    while (true) {

        // to find fallen cs
        std::map<std::string, bool> found_cs;
        for (const auto& it: known_CS) {
           found_cs[it.first] = false;
        }

        auto children = client.get_children(CS_direcrory).get();
        auto real_CS_list = children.children();

        for (const std::string& cs: real_CS_list) {
            auto CS_node = client.get(CS_direcrory + "/" + cs).get();
            std::string CS_addr = get_ip(CS_node) + ":" + std::to_string(get_port(CS_node));

            try {
                if (known_CS.at(cs) != CS_addr) {
                    std::cout << "[CS_Watcher]: " << cs << " changed address" << std::endl;
                    known_CS.at(cs) = CS_addr;
                }

                found_cs[cs] = true;
            } catch (const std::out_of_range& ex) {
                std::cout << "[CS_Watcher]: new znode = " + CS_direcrory + "/" << cs << " | address = "
                    << CS_addr << std::endl;
                known_CS[cs] = CS_addr;
            }
        }

        std::set<std::string> fallen_CS;
        for (const auto& cs: found_cs) {
            if (!cs.second) {
                std::cout << "[CS_Watcher]: " << cs.first << " disconnected" << std::endl;
                known_CS.erase(cs.first);
                fallen_CS.insert(cs.first);
            }
        }

        if (!fallen_CS.empty()) {
            this->renovate(fallen_CS);
        }

        sleep(2); // stop watching for 2 sec
    }
}

void CS_Watcher::renovate(const std::set<std::string> &fallen_cs) {
    for (const auto& CS_id : fallen_cs) {
        const std::string renovation_node = renovation_directory + "/" + CS_id;
        if (!client.exists(renovation_node).get()) {
            client.create(renovation_node, zk::buffer(), zk::create_mode::ephemeral);

            Renovator renovator(this->client, CS_id);
            renovator.run();

            std::cout << "[CS_Watcher]: " << CS_id << " was renovated" << std::endl;
            client.erase(renovation_node);
        }
    }
}

/*
void CS_Watcher::renovate(const std::set<std::string> &fallen_cs) {
    auto meta_file_list = client.get_children(meta_directory).get().children();

    for (const auto& meta_file_name: meta_file_list) {
        std::string full_meta_node_path = meta_directory + "/" + meta_file_name;
        json meta = json::parse(client.get(full_meta_node_path).get().data());

        auto on_cs = meta["on_cs"].get<std::set<std::string>>();
        for (const auto& cs : fallen_cs) {
            if (on_cs.find(cs) != on_cs.end()) {
                auto raid = meta["raid"].get<int32_t>();
                bool status = false;

                if (raid == 0) {
                    Renovator_0 renovator(client,full_meta_node_path, fallen_cs);
                    status = renovator.run();
                }

                if (raid == 1) {
                    Renovator_1 renovator(client, meta_file_name, fallen_cs);
                    status = renovator.run();
                }

                if (!status) {
                    std::cerr << "Error while renovation" << std::endl;
                }
            }
        }
    }
}
*/

Renovator::Renovator(zk::client &client, const std::string& CS_id): client(client), m_CS_id(CS_id) {}

void Renovator::run() {
    auto meta_file_list = client.get_children(meta_directory).get().children();

    for (const auto& meta_file_name: meta_file_list) {
        std::string full_meta_node_path = meta_directory + "/" + meta_file_name; // "Cluster/Meta/<some_file>.meta"
        json meta = json::parse(client.get(full_meta_node_path).get().data());

        auto on_cs = meta["on_cs"].get<std::set<std::string>>();
        if (on_cs.find(m_CS_id) != on_cs.end()) {
            auto raid = meta["raid"].get<int32_t>();

            if (raid == 0) {
                restore_raid_0(meta_file_name);
            }

            if (raid == 1) {
                restore_raid_1((meta_file_name));
            }

            else {
                std::cerr << "Unknown RAID on " << meta_file_name << std::endl;
            }

            on_cs.erase(m_CS_id);
            meta["on_cs"] = m_CS_id;
            std::string str_meta = meta.dump();
            client.set(full_meta_node_path, std::vector<char>(str_meta.begin(), str_meta.end()));
        }
    }
}

void Renovator::restore_raid_0(const std::string &file_node_name) {
    std::cout << "File " << file_node_name.substr(file_node_name.size() - 4)
              << " damaged(RAID 0)" << std::endl;
}

void Renovator::restore_raid_1(const std::string &meta_file_name) {
    const std::string chunk_locations_node_name = meta_directory + "/"                       // "/Cluster/Meta/<some_file>.meta/chunk_locations"
            + meta_file_name + "/chunk_locations";
    auto chunk_locs = client.get_children(chunk_locations_node_name).get().children();

    for (const auto& num_chunk : chunk_locs) {
        auto CS_id_set = json::parse(client.get(chunk_locations_node_name + "/" + num_chunk) // get set of CS_id
                .get().data())["locations"].get<std::set<std::string>>();                    // which current chunk contains

        if (CS_id_set.find(m_CS_id) != CS_id_set.end()) {                                      // check does CS_id contains current chunk
            auto chunk_content = get_chunk_content(CS_id_set, meta_file_name, num_chunk);

            if (!chunk_content.empty()) {
                std::string new_CS_id = send_chunk(CS_id_set, meta_file_name + num_chunk, chunk_content);

                if (!new_CS_id.empty()) {
                    std::cout << "[Renovator]: chunk " << meta_file_name + num_chunk << " copied to "
                        << new_CS_id << std::endl;

                    CS_id_set.erase(m_CS_id);
                    CS_id_set.insert(new_CS_id);

                    json new_data = {"locations", CS_id_set};
                    client.set(chunk_locations_node_name + "/" + num_chunk, new_data);
                }
            }
        }
    }
}

std::vector<char> Renovator::get_chunk_content(const std::set<std::string>& CS_id_set, const std::string& file_UUID,
                                               const std::string& chunk_num) const {
    for (const auto& try_CS_id : CS_id_set) {
        std::string ip;
        uint16_t port;

        try {
            auto CS_info = client.get(CS_direcrory + "/" + try_CS_id).get();
            ip = CS_Watcher::get_ip(CS_info);
            port = CS_Watcher::get_port(CS_info);
        } catch (const zk::no_entry& ex) {continue;}

        rpc::client rpc_client(ip, port);
        sleep(timeout_rpc_connect);
        if (rpc_client.get_connection_state() == rpc::client::connection_state::connected) {
            return rpc_client.call("get_chunk", file_UUID + chunk_num).as<std::vector<char>>();

        } else {
            continue;
        }

    }
    return std::vector<char>();  // return empty vector, if there's no servers to get chunk
}

// send chunk to new CS, returns CS_id where to chunk was written
std::string Renovator::send_chunk(const std::set<std::string>& CS_id_set, const std::string& chunk_UUID,
                       const std::vector<char>& chunk_content) {
    std::vector<std::string> CS_id_list = client.get_children(CS_direcrory).get().children();

    for (const auto& CS_id : CS_id_list) {
        if (CS_id_set.find(CS_id) == CS_id_set.end()) {
            std::string ip;
            uint16_t port;

            try {
                auto CS_info = client.get(CS_direcrory + "/" + CS_id).get();
                ip = CS_Watcher::get_ip(CS_info);
                port = CS_Watcher::get_port(CS_info);
            } catch (const zk::no_entry& ex) {continue;}

            rpc::client rpc_client(ip, port);
            sleep(timeout_rpc_connect);
            if (rpc_client.get_connection_state() == rpc::client::connection_state::connected) {
                rpc_client.call("save_chunk", chunk_UUID, chunk_content);
                return CS_id;
            }
        }
    }

    return std::string();
}

/*
Renovator_0::Renovator_0(zk::client &client, const std::string &meta_node_name,
        const std::set<std::string> &fallen_cs) : Renovator(client, meta_node_name, fallen_cs) {}

bool Renovator_0::run() {
    std::cout << "File " << meta_node_name.substr(meta_node_name.size() - 4)
        << " damaged(RAID 0)" << std::endl;
    return true;
}

Renovator_1::Renovator_1(zk::client &client, const std::string &meta_node_name,
        const std::set<std::string> &fallen_cs) : Renovator(client, meta_node_name, fallen_cs) {
}

bool Renovator_1::run() {
    const std::string chunk_locations_node_name = meta_directory + "/"
            + meta_node_name + "/chunk_locations";
    auto chunk_locs = client.get_children(chunk_locations_node_name).get().children();

    for (const auto& chunk_uuid : chunk_locs) {
        json json_cs_id = json::parse(client.get(chunk_locations_node_name + "/" + chunk_uuid)
                .get().data());
        auto cs_ids = json_cs_id["location"].get<std::set<std::string>>();

        for (const auto& cs_id : cs_ids) {
            if (fallen_cs.find(cs_id) != fallen_cs.end()) {

            }
        }


    }
}
 */