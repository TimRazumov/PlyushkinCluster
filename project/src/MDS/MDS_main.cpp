#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <rpc/client.h>
#include <inttypes.h>
#include <thread>

#include <zk/server/configuration.hpp>
#include <zk/server/server.hpp>
#include <zk/client.hpp>
#include <zk/multi.hpp>
#include <zk/types.hpp>

#include "MDS.h"
#include "CS_Watcher.h"
#include "utils.hpp"

// TODO: обрабатывать, добавлен ли уже такой сервер или нет

void Watcher_thread(CS_Watcher& watcher) {
    watcher.run();
}

int main(int argc, const char *argv[]) {
    const std::string help = "\nYou can use:\n"
                             "help\n"
                             "a   - add ChunkServer\n"
                             "al  - add local ChunkServer\n"
                             "cs  - ChunkServer list\n"
                             "cht - change timeout\n"
                             "gt  - get timeout\n"
                             "st  - stop server\n\n";

    uint16_t rpc_server_port;

    if (argc != 2 || !str_to_uint16(argv[1], rpc_server_port)) {
        rpc_server_port = 0;
    }

    std::cout << "Reading configuration from 'mds_zoo.cfg'" << std::endl;

    MdsConfig mds_config = MDS::read_mds_config("mds_zoo.cfg");

    if (rpc_server_port != 0) {
        mds_config.rpc_server_port = rpc_server_port;
    }

    std::cout << "The rpc server is running. Port: "
        << mds_config.rpc_server_port << std::endl << help;

    MDS this_MDS(mds_config);

    this_MDS.async_run(3);

    auto zk_mds_client = this_MDS.get_zk_client();
    CS_Watcher watcher = CS_Watcher(zk_mds_client);
    std::thread watcher_thread(Watcher_thread, std::ref(watcher));
    watcher_thread.detach();

    std::string command;
    while (true) {
        std::cin >> command;

        if (command == "st" || feof(stdin)) {
            this_MDS.stop();
            //svr.shutdown();
            return 0;
        } else if (command == "help") {
            std::cout << help;
        } else if (command == "a") {
            std::string addr;

            std::cout << "IP: ";
            std::cin >> addr;
            std::cout << "Port: ";
            std::cin >> command;
            uint16_t CS_port;
            if (str_to_uint16(command.c_str(), CS_port)) {
                std::cout << "\nAdded ChunkServer.\n" << "IP: " << addr << "\nPort: " << CS_port << "\n" << std::endl;
                this_MDS.add_CS(addr, CS_port);
            } else {
                std::cout << "Wrong rpc_server_port value" << std::endl;
            }
        } else if (command == "al") {
            std::cout << "Port: ";
            std::cin >> command;
            uint16_t CS_port;
            if (str_to_uint16(command.c_str(), CS_port)) {
                std::cout << "Added local ChunkServer. Port: " << CS_port << std::endl;
                this_MDS.add_CS("127.0.0.1", CS_port);
            } else {
                std::cout << "Wrong rpc_server_port value" << std::endl;
            }
        } else if (command == "cs") {
            auto known_CS = this_MDS.get_known_CS();
            std::cout << "All known ChunkServer:" << std::endl;
            for (const auto &CS : known_CS) {
                std::cout << CS.get_info().addr << " " << CS.get_info().port << std::endl;
            }
        } else if (command == "cht") {
            uint16_t new_timeout;
            std::cout << "New timeout (ms): ";
            std::cin >> command;
            if (str_to_uint16(command.c_str(), new_timeout)) {
                this_MDS.set_cs_timeout(new_timeout);
                std::cout << "Done" << std::endl;
            } else {
                std::cout << "Wrong value" << std::endl;
            }
        } else if (command == "gt") {
            std::cout << "Current timeout (ms): " << this_MDS.get_cs_timeout() << std::endl;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }

}
