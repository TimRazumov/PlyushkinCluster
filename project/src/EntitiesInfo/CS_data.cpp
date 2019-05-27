//
// Created by nickeskov on 21.05.19.
//

#include "EntitiesInfo/CS_data.h"

ClusterCsEntityInfo::ClusterCsEntityInfo(nlohmann::json &json)
                : m_free_id_stack(json["free_id_stack"].get<std::vector<uint32_t>>())
                , m_new_id(json["new_id"].get<uint32_t>()) {

}

nlohmann::json ClusterCsEntityInfo::to_json() {
    return nlohmann::json{
                            {"free_id_stack", m_free_id_stack},
                            {"new_id", m_new_id}
                         };
}


nlohmann::json ClusterCsEntityInfo::get_empty_json() {
    return nlohmann::json{
                            {"free_id_stack", std::vector<uint32_t>()},
                            {"new_id", 0}
                         };
}


uint32_t ClusterCsEntityInfo::get_id() {
    if (m_this_id == (uint32_t) -1) {
        if (m_free_id_stack.empty()) {
            m_this_id = m_new_id++;
        } else {
            m_this_id = m_free_id_stack.back();
            m_free_id_stack.pop_back();
        }
    }
    return m_this_id;
}

ConcreteCsEntityInfo::ConcreteCsEntityInfo(uint16_t port)
                : m_ip(inputIp())
                , m_port(port) {

}

ConcreteCsEntityInfo::ConcreteCsEntityInfo(nlohmann::json &json) : ConcreteCsEntityInfo(std::move(json)) {

}

ConcreteCsEntityInfo::ConcreteCsEntityInfo(nlohmann::json &&json)
    : m_ip(json["ip"].get<std::string>())
    , m_port(json["port"].get<uint16_t>())
    , m_disk_size(json["disk_size"].get<uint64_t>())
    , m_disk_free_space(json["disk_free_space"].get<uint64_t>()) {

}

nlohmann::json ConcreteCsEntityInfo::to_json() {
    return nlohmann::json{
                            {"ip", m_ip},
                            {"port", m_port},
                            {"disk_size", m_disk_size},
                            {"disk_free_space", m_disk_free_space}
                         };
}

std::string ConcreteCsEntityInfo::inputIp() { // TODO(nickeskov): не уверен, что рабоатет
    std::string ip;
    std::cout << "Please, input this CS ip adress" << std::endl;
    std::cin >> ip;

    return ip;
}

std::string &ConcreteCsEntityInfo::get_ip() {
    return m_ip;
}

std::string ConcreteCsEntityInfo::get_ip_copy() {
    return m_ip;
}

uint16_t ConcreteCsEntityInfo::get_port() {
    return m_port;
}

