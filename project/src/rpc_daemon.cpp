#include <iostream>
#include <rpc/client.h>
#include <rpc/server.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/name_generator.hpp>
#include <sys/stat.h>

boost::uuids::uuid fromPath(std::string path) {
    boost::uuids::nil_generator gen;
    boost::uuids::name_generator path_gen(gen());
    return path_gen(path);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        return -1;
    }
    rpc::server srv(2280);

    rpc::client clt(argv[1], 5000);

    srv.bind("getattr", [&clt](const std::string path) {
        auto path_uuid = to_string(fromPath(path));
        auto ret = clt.call("get_attr", path_uuid).as<>();
    });

    srv.bind("readdir", [&clt](const std::string path) {
        auto path_uuid = to_string(fromPath(path));
        auto ret = clt.call("read_dir", path_uuid).as<>();
    });
    
    srv.bind("isDir", [&clt](const std::string path) {

    });

    srv.bind("isFile", [&clt](const std::string path) {

    });

    srv.bind("read", [&clt](const std::string path, size_t size, off_t offset) {

    });

    srv.bind("write", [&clt](const std::string path, const std::string buf,
                             size_t size, off_t offset) {
    
    });
    srv.run();
}
