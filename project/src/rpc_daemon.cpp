#include <iostream>
#include <rpc/client.h>
#include <rpc/server.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/name_generator.hpp>
#include <sys/stat.h>

#include "utils.hpp"

const int TIMEOUT = 5000;
const int LOCAL_PORT = 2280;
const int MDS_PORT = 5000;
constexpr size_t CHUNK_SIZE = 2*1024*1024;


int min(int a, int b) {
    return a < b ? a : b;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return -1;
    }
    rpc::server srv(LOCAL_PORT);

    rpc::client clt(argv[1], MDS_PORT);
    
    clt.set_timeout(TIMEOUT);

    srv.bind("getattr", [&clt](const std::string path) {
        auto path_uuid = uuid_from_str(path);
        auto ret = clt.call("get_attr", path_uuid).as<std::vector<std::string>>();
        std::vector<int> attrs;
        for (auto i : ret) {
            attrs.push_back(std::stoi(i));
        }
        return attrs;
    });

    srv.bind("readdir", [&clt](const std::string path) {
        auto path_uuid = uuid_from_str(path);
        int i = 0;
        auto ret = clt.call("get_chunk", path_uuid, i).as<std::string>();
        std::vector<std::string> ret_vec;
        if (ret.size() == 0) {
            return ret_vec;
        }
        do {
            std::istringstream str(ret);
            std::string buf;
            while (std::getline(str, buf)) {
                ret_vec.push_back(buf);
            }
            i++;
            ret.clear();
            ret = clt.call("get_chunk", path_uuid, i).as<std::string>();
        } while (ret.size() != 0);
        return ret_vec;
    });
    
    srv.bind("isDir", [&clt](const std::string path) {

    });

    srv.bind("isFile", [&clt](const std::string path) {
        auto path_uuid = uuid_from_str(path);
        auto ret = clt.call("get_attr", path_uuid).as<std::vector<std::string>>();
        if (ret.size() != 0 &&
            std::stoi(ret[0]) >= 0) {
            return true;
        }
        return false;
    });

    srv.bind("read", [&clt](const std::string path, size_t size, off_t offset) {
        auto path_uuid = uuid_from_str(path);
        int chunk_number = offset/CHUNK_SIZE;
        int local_offset = offset % CHUNK_SIZE;
        int bias = 0;
        auto ret = clt.call("get_chunk", path_uuid, chunk_number).as<std::string>();
        std::string ret_str = "";
        do {
            ret_str += ret.substr(local_offset, min(size, CHUNK_SIZE - local_offset));
            size -= ret_str.size();
            bias++;
            local_offset = 0;
            ret.clear();
            ret = clt.call("get_chunk", path_uuid,
                    chunk_number +bias).as<std::string>();
        } while (size > 0 && ret.size() > 0);
        return ret_str;
    });

    srv.bind("write", [&clt](const std::string path, std::string buf,
                             size_t size, off_t offset) {
        auto path_uuid = uuid_from_str(path);
        int chunk_number = offset / CHUNK_SIZE;
        int loc_offset = offset % CHUNK_SIZE;
        int bias = 0;
        auto ret = clt.call("get_chunk", path_uuid, chunk_number).as<std::string>();
        std::string chunk;
        do {
            ret.insert(loc_offset, buf);
            chunk = ret.substr(0, CHUNK_SIZE);
            clt.call("save_chunk", path_uuid, chunk_number + bias, chunk).as<bool>();
            if (ret.size() < CHUNK_SIZE) {
                break;
            }
            buf = ret.substr(CHUNK_SIZE, ret.size());
            bias++;
            loc_offset = 0;
            ret = clt.call("get_chunk", path_uuid, chunk_number + bias).as<std::string>();
        } while (true);
        return true;
    });

    srv.bind("mknod", [&clt](const std::string path) {
        auto path_uuid = uuid_from_str(path);
        std::vector<std::string> attrs;
        attrs.push_back(std::to_string(0));
        return clt.call("set_attr", path_uuid, attrs).as<bool>();
    });
    
    srv.bind("delete_file", [&clt](const std::string path) {
        return clt.call("delete_file", uuid_from_str(path)).as<bool>();
    });

    srv.bind("rename", [&clt](const std::string from, const std::string to) {
    
    });
    
    srv.bind("mkdir", [&clt](const std::string path) {

    });
    
    srv.bind("rmdir", [&clt](const std::string path) {

    });

    srv.run();
}
