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

std::vector<char>::iterator line_end(std::vector<char>::iterator it) {
    while (*it != '\n')
        it++;
    return it;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return -1;
    }
    rpc::server srv(LOCAL_PORT);

    rpc::client clt(argv[1], MDS_PORT);
    
    clt.set_timeout(TIMEOUT);
    auto getattr = [&](const std::string path)->std::vector<int> {
        auto path_uuid = uuid_from_str(path);
        auto ret = clt.call("get_attr", path_uuid).as<std::vector<std::string>>();
        std::vector<int> attrs;
        for (auto i : ret) {
            attrs.push_back(std::stoi(i));
        }
        return attrs;
    };


    auto readdir = [&](const std::string path) {
        auto path_uuid = uuid_from_str(path);
        int i = 0;
        auto ret = clt.call("get_chunk", path_uuid, i).as<std::vector<char>>();
        std::vector<std::string> ret_vec;
        auto iter = ret.begin();
        if (ret.empty()) {
            return ret_vec;
        }
        do {
            auto tmp_iter = line_end(iter);
            ret_vec.push_back(std::string(iter, tmp_iter));
            iter = tmp_iter + 1;
            i++;
            ret = clt.call("get_chunk", path_uuid, i).as<std::vector<char>>();
        } while (!ret.empty());
        return ret_vec;
    };
    
    auto isDir = [&](const std::string path) {

    };

    auto isFile = [&](const std::string path) {
        auto path_uuid = uuid_from_str(path);
        auto ret = clt.call("get_attr", path_uuid).as<std::vector<std::string>>();
        if (ret.size() != 0 &&
            std::stoi(ret[0]) >= 0) {
            return true;
        }
        return false;
    };

    auto read = [&](const std::string path, size_t size, off_t offset)->std::string {
        int file_size = getattr(path)[0];
        if (offset >= file_size) {
            return "";
        }
        if (size + offset > file_size) {
            size = file_size - offset;
        }
        int max_chunks = file_size / CHUNK_SIZE; 
        int chunk_number = offset / CHUNK_SIZE;
        int local_offset = offset % CHUNK_SIZE;
        auto path_uuid = uuid_from_str(path);
        std::string ret_str = "";
        do {
            auto ret = clt.call("get_chunk", path_uuid, chunk_number).as<std::vector<char>>();
            ret_str += std::string(ret.begin() + local_offset,
                    ret.begin() + local_offset + 
                    min(size, CHUNK_SIZE - local_offset));
            size -= ret_str.size();
            local_offset = 0;
            chunk_number++;
        } while (size > 0);
        return ret_str;
    };

    auto write = [&](const std::string path, std::string buf,
                             size_t size, off_t offset) {
        auto attrs = getattr(path);
        size_t file_size = attrs[0];
        int max_chunks = file_size / CHUNK_SIZE; 
        int chunk_number = offset / CHUNK_SIZE;
        int loc_offset = offset % CHUNK_SIZE;
        auto path_uuid = uuid_from_str(path);
        std::vector<char> ret;
        while (!buf.empty()) {
            if (chunk_number <= max_chunks) {
                ret = clt.call("get_chunk", path_uuid, chunk_number).as<std::vector<char>>();
            }
            ret.insert(ret.begin() + loc_offset, buf.begin(), buf.end());
            auto chunk = std::vector<char>(ret.begin(), ret.begin() + CHUNK_SIZE);
            chunk_number++;
            clt.call("save_chunk", path_uuid, chunk_number, chunk).as<bool>();
            if (ret.size() > CHUNK_SIZE) {
                buf = std::string(ret.begin() + CHUNK_SIZE, ret.end());
            } else {
                buf = "";
            }
            loc_offset = 0;
            ret.clear();
        }
        std::vector<std::string> set_attrs;
        set_attrs.push_back(std::to_string(file_size + size));
        return clt.call("set_attr", path_uuid, set_attrs).as<bool>();
    };

    auto mknod = [&](const std::string path) {
        auto path_uuid = uuid_from_str(path);
        std::vector<std::string> attrs;
        attrs.push_back(std::to_string(0));
        return clt.call("set_attr", path_uuid, attrs).as<bool>();
    };
    
    auto delete_file = [&](const std::string path) {
        return clt.call("delete_file", uuid_from_str(path)).as<bool>();
    };

    auto rename = [&](const std::string from, const std::string to) {
    
    };
    
    auto mkdir = [&](const std::string path) {

    };
    
    auto rmdir = [&](const std::string path) {

    };

    srv.bind("getattr", getattr);
    srv.bind("readdir", readdir);
    srv.bind("isDir", isDir);
    srv.bind("isFile", isFile);
    srv.bind("read", read);
    srv.bind("write", write);
    srv.bind("mknod", mknod);
    srv.bind("delete_file", delete_file);
    srv.bind("rename", rename);
    srv.bind("mkdir", mkdir);
    srv.bind("rmdir", rmdir);

    srv.run();
}
