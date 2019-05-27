#ifndef PLYUSHKINCLUSTER_KNOWN_MDS_H
#define PLYUSHKINCLUSTER_KNOWN_MDS_H

#include <stdint.h>
#include <vector>
#include <string>
#include <inttypes.h>
#include <set>
#include <nlohmann/json.hpp>

enum MDS_status {slave, master};

// все данные об MDS сервере, которые могут понадобиться остальным кускам кода
struct MDS_info {
    // сервер master/slave
    MDS_status status;

    // ms
    int64_t timeout;

    MDS_info(const MDS_status &status, const int64_t &timeout) : status(status), timeout(timeout) {}
};


class MDS_data {
private:
    MDS_info info;
public:
    MDS_data(const MDS_status &status, const int64_t &timeout) : info(status, timeout) {}
    MDS_info const get_info() { return info; }

    void change_timeout(const int64_t &new_timeout) { info.timeout = new_timeout; }
    void change_status() { info.status == master ? info.status = slave : info.status = master; }
};


class MetaEntityInfo final {
public:
    explicit MetaEntityInfo(nlohmann::json &json);
    explicit MetaEntityInfo(nlohmann::json &&json);
    // explicit MetaEntityInfo(std::vector<std::string> &info); // For new node

    nlohmann::json to_json();

    std::vector<std::string> &get_attr();
    std::vector<std::string> get_attr_copy();
    void set_attr(std::vector<std::string> &attr);

    int32_t get_raid();

    std::set<uint32_t> &get_on_cs();
    std::set<uint32_t> get_on_cs_copy();
private:
    std::vector<std::string> m_attr;
    int32_t m_raid = 1; // TODO(nickeskov): make changeable later
    std::set<uint32_t> m_on_cs;
};

class ChunkEntityInfo final {
public:
    explicit ChunkEntityInfo(nlohmann::json &json);
    explicit ChunkEntityInfo(nlohmann::json &&json);

    nlohmann::json to_json();

    std::vector<uint32_t> &get_locations();
    std::vector<uint32_t> get_locations_copy();

private:
    std::vector<uint32_t> m_locations;
};

#endif //PLYUSHKINCLUSTER_KNOWN_MDS_H
