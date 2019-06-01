#include <iostream>
#include <rpc/client.h>
#include <rpc/server.h>
#include <rpc/rpc_error.h>
#include <rpc/this_handler.h>

#include "utils.hpp"

const int TIMEOUT = 20000;
const int LOCAL_PORT = 2280;
constexpr size_t CHUNK_SIZE = 2*1024*1024;

using err_t = std::tuple<int, std::string>;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Invalid input: IP port";
        return -1;
    }
    
    rpc::server srv(LOCAL_PORT);

    rpc::client clt(argv[1], std::stoi(argv[2]));
    
    clt.set_timeout(TIMEOUT);
    
    srv.suppress_exceptions(true);

    auto init = [&]() {
        if (!clt.call("is_meta_exist", uuid_from_str("/")).as<bool>()) {
            std::vector<unsigned int> attrs = {0, 2, 5, 5, 7};
            clt.call("set_attr", uuid_from_str("/"), attrs_to_string(attrs));
        }
    };
    
    auto access = [&](const std::string path) {
        try {
            std::cout << "access: " << path << std::endl;
            if (clt.call("is_meta_exist", uuid_from_str(path)).as<bool>()) {
                return 0;
            }
            return -1;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };

    auto getattr = [&](const std::string path)->std::vector<unsigned int> {
        try {
            std::cout << "getattr: " << path << std::endl;
            auto path_uuid = uuid_from_str(path);
            auto ret = clt.call("get_attr", path_uuid).as<std::vector<std::string>>();
            std::vector<unsigned int> attrs;
            for (const auto &i : ret) {
                attrs.push_back(std::stoi(i));
            }
            return attrs;
        } catch (rpc::rpc_error& e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };


    auto readdir = [&](const std::string path) {
        try {
            std::cout << "readdir: " << path << std::endl;
            auto attrs = getattr(path);
            std::vector<std::string> ret_vec;
            if (attrs[0] == 0) {
                return ret_vec;
            }
            size_t max_chunks = attrs[0] / CHUNK_SIZE;
            auto path_uuid = uuid_from_str(path);
            size_t i = 0;
            while (i <= max_chunks) {
                auto ret = clt.call("get_chunk", path_uuid, i).as<std::vector<char>>();
                auto iter = ret.begin();
                auto it_end = ret.end();
                while (iter < it_end) {
                    auto tmp_iter = line_end(iter);
                    ret_vec.push_back(std::string(iter, tmp_iter));
                    iter = tmp_iter + 1;
                }
                i++;
            }
            return ret_vec;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };

    auto isFile = [&](const std::string path) {
        try {
            std::cout << "isFile: " << path << std::endl;
            auto path_uuid = uuid_from_str(path);
            auto ret = clt.call("get_attr", path_uuid).as<std::vector<std::string>>();
            if (ret.size() != 0 &&
                std::stoi(ret[1]) == 1) {
                return true;
            }
            return false;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };
    
    auto isDir = [&](const std::string path) {
        try {
            std::cout << "isDir: " << path << std::endl;
            return !isFile(path);
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occurred"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };

    auto read = [&](const std::string path, size_t size, off_t offset) {
        try {
            std::cout << "read: " << path << " " << size << " " << offset << std::endl;
            size_t file_size = getattr(path)[0];
            std::vector<char> ret_str;
            if (offset >= file_size) {
                return ret_str;
            }
            if ((int)size + offset > file_size) {
                size = file_size - (size_t)offset;
            }
            size_t max_chunks = file_size / CHUNK_SIZE; 
            size_t chunk_number = offset / CHUNK_SIZE;
            size_t local_offset = offset % CHUNK_SIZE;
            auto path_uuid = uuid_from_str(path);
            do {
                auto ret = clt.call("get_chunk", path_uuid, chunk_number).as<std::vector<char>>();
                ret_str.insert(ret_str.end(), ret.begin() + local_offset, ret.begin() + 
                               std::min<size_t>(CHUNK_SIZE, local_offset + size));
                size -= ret_str.size();
                local_offset = 0;
                chunk_number++;
            } while (size > 0 && chunk_number <= max_chunks);
            return ret_str;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occurred"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };

    auto write = [&](const std::string path, std::vector<char> buf,
                     size_t size, off_t offset) {
        try {
            std::cout << "write: ";
            std::cout << path << " ";
            std::cout << buf.data() << " ";
            std::cout << size << " ";
            std::cout << offset << std::endl;
            auto attrs = getattr(path);
            size_t file_size = attrs[0];
            attrs[0] = offset + size;
            auto path_uuid = uuid_from_str(path);
            clt.call("set_attr", path_uuid,
                    attrs_to_string(attrs));
            size_t max_chunks = file_size / CHUNK_SIZE; 
            size_t chunk_number = offset / CHUNK_SIZE;
            size_t loc_offset = offset % CHUNK_SIZE;
            std::vector<char> ret;
            if (file_size != 0 && chunk_number <= max_chunks) {
                ret = clt.call("get_chunk", path_uuid, chunk_number).as<std::vector<char>>();
            }
            ret.insert(ret.begin() + loc_offset, buf.begin(), buf.end());
            ret.resize(loc_offset + size);
            size_t ret_chunks_count = ret.size() / CHUNK_SIZE;
            for (size_t i = 0; i <= ret_chunks_count; i++) {
                auto chunk = std::vector<char>(ret.begin(),
                             ret.begin() + std::min<int>(ret.size(), CHUNK_SIZE));
                clt.call("save_chunk", path_uuid, chunk_number, chunk);
                chunk_number++;
                if (ret.size() > CHUNK_SIZE) {
                    ret = std::vector<char>(ret.begin() + CHUNK_SIZE, ret.end());
                }
            }
            std::cout << "write end" << std::endl;
            return size;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };


    auto mknod = [&](const std::string path, std::vector<unsigned int> attrs) -> bool {
        try {
            auto path_uuid = uuid_from_str(path);
            clt.call("set_attr", path_uuid, attrs_to_string(attrs));

            auto cur_dir = getDirByPath(path);
            auto filename = nameByPath(path) + '\n';
            auto dir_attrs = getattr(cur_dir);
            return write(cur_dir, std::vector<char>(filename.begin(),
                         filename.end()), filename.size(), dir_attrs[0]);
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };

    auto delete_from_dir = [&](std::string cur_dir, std::string filename) {
        int cur_chunk = 0;
        std::string chunk;
        std::vector<char> buf;
        auto list_of_files = readdir(cur_dir);
        for (auto i : list_of_files) {
            if (i == filename)
                continue;
            chunk += i + '\n';
        }
        return chunk;
    };
    
    auto delete_file = [&](const std::string path) {
        try {
            auto cur_dir = getDirByPath(path);
            auto filename = nameByPath(path);
            auto dir_attrs = getattr(cur_dir);
            auto chunk = delete_from_dir(cur_dir, filename);
            write(cur_dir, std::vector<char>(chunk.begin(), chunk.end()), chunk.size(), 0);
            clt.call("delete_file", uuid_from_str(path));
            return true;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };

    auto rename = [&](const std::string from, const std::string to) {
        try {
            std::cout << "rename: " << from << " " << to << std::endl;
            
            auto from_filename = nameByPath(from);
            auto from_dir = getDirByPath(from);
            auto from_attrs = getattr(from_dir);
            auto chunk = delete_from_dir(from_dir, from_filename);
            std::cout << from_filename << from_dir << chunk << std::endl;
            write(from_dir, std::vector<char>(chunk.begin(), chunk.end()), chunk.size(), 0);
            auto to_dir = getDirByPath(to);
            auto to_filename = nameByPath(to) + '\n';
            auto to_attrs = getattr(to_dir);
            std::cout << to_dir << to_filename << std::endl;
            write(to_dir, std::vector<char>(to_filename.begin(), to_filename.end()), to_filename.size(), to_attrs[0]);
            clt.call("rename_file", uuid_from_str(from), uuid_from_str(to));
            return true;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };

    auto chmod = [&](const std::string path, std::vector<unsigned int> perms) {
        try {
            auto attrs = getattr(path);
            for (int i = 0; i < 3; i++) 
                attrs[2+i] = perms[i];
            clt.call("set_attr", uuid_from_str(path), attrs_to_string(attrs));
            return true;
        } catch (rpc::rpc_error &e) {
            auto err = e.get_error().as<err_t>();
            if (std::get<0>(err) == 1) {
                rpc::this_handler().respond_error(
                    std::make_tuple(3, "File is not exist"));
            }
            rpc::this_handler().respond_error(
                std::make_tuple(2, "MDS error occured"));
        } catch (rpc::timeout &t) {
            rpc::this_handler().respond_error(
                std::make_tuple(1, "MDS timeout"));
        }
    };
    
    srv.bind("access", access);
    srv.bind("getattr", getattr);
    srv.bind("readdir", readdir);
    srv.bind("isDir", isDir);
    srv.bind("isFile", isFile);
    srv.bind("read", read);
    srv.bind("write", write);
    srv.bind("mknod", mknod);
    srv.bind("delete_file", delete_file);
    srv.bind("rename", rename);
    srv.bind("init", init);
    srv.bind("chmod", chmod);
    srv.run();
}
