#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <rpc/client.h>
#include <inttypes.h>

#include <zk/server/configuration.hpp>
#include <zk/server/server.hpp>
#include <zk/client.hpp>
#include <zk/multi.hpp>
#include <zk/types.hpp>

#include "MDS.h"
#include "utils.hpp"

const std::vector<char> new_data = {' '};

template <typename T>
void print_thing(std::future<T>&& result)
{
  try
  {
    // Unwrap the future value, which will not block (based on usage), but could throw.
    T value(result.get());
    std::cerr << value << std::endl;
  }
  catch (const std::exception& ex)
  {
    // Error "handling"
    std::cerr << "Exception: " << ex.what() << std::endl;
  }
}

// TODO: обрабатывать, добавлен ли уже такой сервер или нет
const std::string help = "\nYou can use:\n"
                         "help\n"
                         "a   - add CS\n"
                         "al  - add local CS\n"
                         "cs  - CS list\n"
                         "cht - change timeout\n"
                         "gt  - get timeout\n"
                         "chs - change status\n"
                         "gs  - get status\n"
                         "st  - stop server\n\n";


int main(int argc, const char *argv[]) {
    uint16_t port;

    if (argc != 2 || !str_to_uint16(argv[1], port)) {
        std::cout << "Usage: " << argv[0] << " port\n";
        return 1;
    }

    auto config = zk::server::configuration::make_minimal("zk-data", 2821)
                    .add_server(1, "192.168.3.9");

    config.save_file("settings.cfg");
    {
        std::ofstream of("zk-data/myid");
        // Assuming this is server 1
        of << 1 << std::endl;
    }

    zk::server::server svr(config);

    auto client = zk::client::connect("zk://192.168.3.9:2821").get();

    client.create("/Cluster/MDS/MDS_", new_data, zk::create_mode::sequential | zk::create_mode::ephemeral);
    std::cout << "\n\n\n";
    print_thing(client.get_children("/"));
    std::cout << std::endl;
    print_thing(client.get_children("/Cluster/MDS"));

    std::cout << "The server is running. Port: " << port << std::endl << help;

    MDS this_MDS(port);

    this_MDS.async_run(3);

    std::string command;
    while (true) {
        std::cin >> command;

        if (command == "st" || feof(stdin)) {
            this_MDS.stop();
            //svr.shutdown();
            client.close();
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
                std::cout << "\nAdded CS.\n" << "IP: " << addr << "\nPort: " << CS_port << "\n" << std::endl;
                this_MDS.add_CS(addr, CS_port);
            } else {
                std::cout << "Wrong port value" << std::endl;
            }
        } else if (command == "al") {
            std::cout << "Port: ";
            std::cin >> command;
            uint16_t CS_port;
            if (str_to_uint16(command.c_str(), CS_port)) {
                std::cout << "Added local CS. Port: " << CS_port << std::endl;
                this_MDS.add_CS("127.0.0.1", CS_port);
            } else {
                std::cout << "Wrong port value" << std::endl;
            }
        } else if (command == "cs") {
            auto known_CS = this_MDS.get_known_CS();
            std::cout << "All known CS:" << std::endl;
            for (const auto &CS : known_CS) {
                std::cout << CS.get_info().addr << " " << CS.get_info().port << std::endl;
            }
        } else if (command == "cht") {
            uint16_t new_timeout;
            std::cout << "New timeout (ms): ";
            std::cin >> command;
            if (str_to_uint16(command.c_str(), new_timeout)) {
                this_MDS.change_timeout(new_timeout);
                std::cout << "Done" << std::endl;
            } else {
                std::cout << "Wrong value" << std::endl;
            }
        } else if (command == "gt") {
            std::cout << "Current timeout (ms): " << this_MDS.get_timeout() << std::endl;
        } else if (command == "chs") {
            this_MDS.change_status();
            std::cout << "IN PROGRESS" << std::endl;
        } else if (command == "gs") {
            std::cout << "Current status: " << this_MDS.get_status() << std::endl;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }
}
