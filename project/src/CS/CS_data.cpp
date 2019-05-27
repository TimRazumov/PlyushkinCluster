//
// Created by nickeskov on 21.05.19.
//

#include "CS_data.h"

ClusterCsData::ClusterCsData(nlohmann::json json)
                : m_free_id_stack(json["free_id_stack"].get<std::vector<uint32_t>>())
                , m_new_id(json["new_id"].get<uint32_t>()) {

}

nlohmann::json ClusterCsData::get_data() {
    return nlohmann::json{
                            {"free_id_stack", m_free_id_stack},
                            {"new_id", m_new_id}
                         };
}

uint32_t ClusterCsData::get_id() {
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

ConcreteCsData::ConcreteCsData(uint16_t port)
                : m_ip(getIp())
                , m_port(port) {

}

nlohmann::json ConcreteCsData::get_data() {
    return nlohmann::json{
                            {"ip", m_ip},
                            {"port", m_port},
                            {"disk_free_space", m_disk_free_space},
                            {"disk_size", m_disk_size}
                         };
}

std::string ConcreteCsData::getIp() { // TODO(nickeskov): не уверен, что рабоатет
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket(io_service);

    std::string ip = socket.remote_endpoint().address().to_string();
    return ip;
}

