#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
//#include <chrono>

#include "CS.h"


//using namespace std::chrono_literals;

int main() {
    if (boost::filesystem::create_directories(CS_DIRECTORY)) {
        std::cout << "mkdir " << CS_DIRECTORY << std::endl;
    }

    rpc::server this_CS(8080);

    this_CS.bind(
            "stop_server", [&this_CS]() {
                this_CS.stop();
                return true;
            }
    );

    this_CS.bind(
            "save_chunk", [](std::string const &chunk_UUID, std::string const &chunk_content) {
                std::ofstream chunk_file(std::string(CS_DIRECTORY) + "/" + chunk_UUID);

                chunk_file << CHUCK_NAME << std::endl << chunk_content;

                return true;
            }
    );

    this_CS.bind(
            "download_chunk", [](std::string const &chunk_UUID) {
                std::ifstream chunk_file(std::string(CS_DIRECTORY) + "/" + chunk_UUID);
                std::string result, buffer;

                while (std::getline(chunk_file, buffer)) {
                    result += buffer + "\n";
                }

                return result;
            }
    );

    this_CS.run();
    return 0;
}
