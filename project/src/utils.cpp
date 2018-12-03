#include "utils.hpp"

bool isFile(std::string const &path) {
    struct stat buf;
    stat(path.c_str(), &buf);
    if (S_ISREG(buf.st_mode)) {
        return true;
    } else {
        return false;
    }
}

bool isDir(std::string const &path) {
    struct stat buf;
    stat(path.c_str(), &buf);
    if (S_ISDIR(buf.st_mode)) {
        return true;
    } else {
        return false;
    }
}

std::string uuid_from_str(std::string const &path) {
    boost::uuids::string_generator ns;
    boost::uuids::uuid dns_namespace_uuid 
        = ns("{6ba7b810-9dad-11d1-80b4-00c04fd430c8}");
    boost::uuids::name_generator gen(dns_namespace_uuid);
    boost::uuids::uuid u = gen(path);
    return to_string(u);
}
