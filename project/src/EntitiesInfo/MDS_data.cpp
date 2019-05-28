//
// Created by nickeskov on 27.05.19.
//

#include "EntitiesInfo/MDS_data.h"

MetaEntityInfo::MetaEntityInfo(nlohmann::json &&json)
    : m_attr(json["attr"].get<std::vector<std::string>>())
    , m_raid(json["raid"].get<int32_t>())
    , m_on_cs(json["on_cs"].get<std::set<uint32_t>>()) {

}

MetaEntityInfo::MetaEntityInfo(nlohmann::json &json) : MetaEntityInfo(std::move(json)) {

}


nlohmann::json MetaEntityInfo::get_empty_json() {
    return nlohmann::json{
            {"attr", std::vector<std::string>()},
            {"raid", 1},
            {"on_cs", std::set<uint32_t >()}
    };
}

nlohmann::json MetaEntityInfo::to_json() {
    return nlohmann::json{
                            {"attr", m_attr},
                            {"raid", m_raid},
                            {"on_cs", m_on_cs}
                         };
}

std::vector<std::string> &MetaEntityInfo::get_attr() {
    return m_attr;
}

std::vector<std::string> MetaEntityInfo::get_attr_copy() {
    return m_attr;
}

void MetaEntityInfo::set_attr(std::vector<std::string> &attr) {
    m_attr = std::move(attr);
}


int32_t MetaEntityInfo::get_raid() {
    return m_raid;
}

std::set<uint32_t> &MetaEntityInfo::get_on_cs() {
    return m_on_cs;
}

std::set<uint32_t> MetaEntityInfo::get_on_cs_copy() {
    return m_on_cs;
}

// ------------------------------------------------------

ChunkEntityInfo::ChunkEntityInfo(nlohmann::json &&json)
    : m_locations(json["locations"].get<std::set<uint32_t>>()) {

}

nlohmann::json ChunkEntityInfo::get_empty_json() {
    return nlohmann::json{
                            {"locations", std::set<uint32_t>()}
                         };
}

ChunkEntityInfo::ChunkEntityInfo(nlohmann::json &json) : ChunkEntityInfo(std::move(json)) {

}

nlohmann::json ChunkEntityInfo::to_json() {
    return nlohmann::json {
                            {"locations", m_locations}
                          };
}

std::set<uint32_t> &ChunkEntityInfo::get_locations() {
    return m_locations;
}

std::set<uint32_t> ChunkEntityInfo::get_locations_copy() {
    return m_locations;
}
