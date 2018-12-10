#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <rpc/client.h>
#include <inttypes.h>

#include "MDS.h"


const std::string help = "\nYou can use:\n"
                         "help\n"
                         "al - add local CS\n"
                         "cs - CS list\n"
                         "cht - change timeout\n"
                         "gt - get timeout\n"
                         "chs - change status\n"
                         "gs - get status\n"
                         "st - stop server\n\n";


// TODO: move add_log and str_to_uint16 to utils
static bool str_to_uint16(const char *str, uint16_t &res) {
    char *end;
    errno = 0;
    intmax_t val = strtoimax(str, &end, 10);
    if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0')
        return false;
    res = (uint16_t) val;
    return true;
}


int main(int argc, const char *argv[]) {
    uint16_t port;

    if (argc != 2 || !str_to_uint16(argv[1], port)) {
        std::cout << "Usage: " << argv[0] << " port\n";
        return 1;
    }

    std::cout << "The server is running. Port: " << port << std::endl << help;

    MDS this_MDS(port);

    this_MDS.async_run(3);

    std::string command;
    while (true) {
        std::cin >> command;

        if (command == "st" || feof(stdin)) {
            this_MDS.stop();
            return 0;
        } else if (command == "help") {
            std::cout << help;
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
                std::cout << "DONE, SIR" << std::endl;
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
