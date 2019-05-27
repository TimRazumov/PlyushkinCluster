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




class ClusterCsEntityInfo final {
public:
    explicit ClusterCsEntityInfo(nlohmann::json &json);

    uint32_t get_id();
    nlohmann::json to_json();

    static nlohmann::json get_empty_json();

private:
    std::vector<uint32_t> m_free_id_stack;
    uint32_t m_new_id = -1;
    uint32_t m_this_id = -1;
};


class ConcreteCsEntityInfo final {
public:
    explicit ConcreteCsEntityInfo(uint16_t port);
    explicit ConcreteCsEntityInfo(nlohmann::json &json);
    explicit ConcreteCsEntityInfo(nlohmann::json &&json);

    std::string& get_ip();
    std::string get_ip_copy();
    uint16_t get_port();

    nlohmann::json to_json();
    std::string inputIp();
private:
    std::string m_ip;
    uint16_t m_port = 0;
    uint64_t m_disk_size = 0;
    uint64_t m_disk_free_space = 0;
};

#endif //PLYUSHKINCLUSTER_KNOWN_CS_H
