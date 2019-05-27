#ifndef PLYUSHKINCLUSTER_KNOWN_CS_H
#define PLYUSHKINCLUSTER_KNOWN_CS_H

#include <vector>
#include <iostream>
#include <ifaddrs.h>

#include <nlohmann/json.hpp>

typedef struct {
    std::string addr;
    uint16_t port = 0;
} CS_info;


class CS_data {
private:
    CS_info info;
public:
    CS_data(const std::string &addr, uint16_t port) : info{addr, port} {}
    ~CS_data() = default;
    CS_info get_info() const { return info; }
};




class ClusterCsData final {
public:
    explicit ClusterCsData(nlohmann::json &json);

    uint32_t get_id();
    nlohmann::json get_data();

    static nlohmann::json get_empty_json();

private:
    std::vector<uint32_t> m_free_id_stack;
    uint32_t m_new_id = -1;
    uint32_t m_this_id = -1;
};


class ConcreteCsData final {
public:
    explicit ConcreteCsData(uint16_t port);

    nlohmann::json get_data();
    std::string getIp();
private:
    std::string m_ip;
    uint16_t m_port = 0;
    uint64_t m_disk_size = 0;
    uint64_t m_disk_free_space = 0;
};

#endif //PLYUSHKINCLUSTER_KNOWN_CS_H
