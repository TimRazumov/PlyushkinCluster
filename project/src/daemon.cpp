#include <vector>
#include <string>
#include <iostream>
#include "rpc/client.h"
#include "rpc/server.h"

#include <stdlib.h>

#include "utils.hpp"

#define THIS_SERVER_PORT 2280
#define MDS_PORT 5000
#define MDS_IP "127.0.0.1"
#define TIMEOUT 1000



int main() {
    rpc::server srv(THIS_SERVER_PORT);
    rpc::client mds(MDS_IP, MDS_PORT);
    mds.set_timeout(TIMEOUT);


    srv.bind("getattr", 
        [&mds](std::string const path) {
            std::string uuid = uuid_from_str(path);
            auto attrs = mds.call("get_attr", uuid).as<std::vector<std::string>>();
            std::vector<int> attri;
            for (std::string n : attrs)
                attri.push_back(atoi(n.c_str()));
            return attri;
        });
    
    srv.bind("readdir", [&mds](const std::string path) {
     //   auto ret = mds.call("read_dir", path_uuid).as<>();
    }); 
    
    srv.bind("isDir", [&mds](const std::string path) {

    }); 

    srv.bind("isFile", [&mds](const std::string path) {

    });

    srv.bind("read", [&mds](const std::string path, size_t size, off_t offset) {

    });

    srv.bind("write", [&mds](const std::string path, const std::string buf,
                             size_t size, off_t offset) {

    });


    srv.run();

    return 0;
}
