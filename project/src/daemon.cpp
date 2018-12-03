#include <vector>
#include <string>
#include <iostream>
#include "rpc/client.h"
#include "rpc/server.h"

#include <stdlib.h>

#include "utils.hpp"

#define THIS_SERVER_PORT 1489
#define MDS_PORT 1488
#define MDS_IP "127.0.0.1"
#define TIMEOUT 1000



int main() {
    rpc::server this_srv("0.0.0.0", THIS_SERVER_PORT);
    rpc::client mds(MDS_IP, MDS_PORT);
    mds.set_timeout(TIMEOUT);


    this_srv.bind("getattr", 
        [&mds](std::string const path) {
            std::string uuid = uuid_from_str(path);
            auto attrs = mds.call("get_attr", uuid).as<std::vector<std::string>>();
            std::vector<int> attri;
            for (std::string n : attrs)
                attri.push_back(atoi(n.c_str()));
            return attri;
        });
    

    this_srv.run();

    return 0;
}
