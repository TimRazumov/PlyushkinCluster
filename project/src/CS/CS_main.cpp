#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <ctime>

#include "CS_utils.h"


// TODO: безопасность

int main() {
    if (boost::filesystem::create_directories(CS_DIRECTORY)) {
        std::cout << "mkdir " << CS_DIRECTORY << std::endl;
    }



    rpc::server this_CS(8080);

    this_CS.bind(
            "stop_server", [&this_CS]() {
                std::ofstream log_file(std::string(CS_DIRECTORY) + "/" + "log", std::ios_base::app);
                log_file << "s";
                log_file.close();

                this_CS.stop();
                return true;
            }
    );

    this_CS.bind(
            "save_chunk", [](const std::string &chunk_UUID, const std::string &chunk_content) {
                std::ofstream log_file(std::string(CS_DIRECTORY) + "/" + "log", std::ios_base::app);
                time_t seconds = time(nullptr);
                tm* time_info = localtime(&seconds);
                log_file << asctime(time_info) << " save_chunk\n";
                log_file.close();


                std::ofstream chunk_file(std::string(CS_DIRECTORY) + "/" + chunk_UUID);

                chunk_file << chunk_content;


                log_file.close();
                return true;
            }
    );

    this_CS.bind(
            "download_chunk", [](const std::string &chunk_UUID) {
                std::ofstream log_file(std::string(CS_DIRECTORY) + "/" + "log", std::ios_base::app);
                time_t seconds = time(nullptr);
                tm* time_info = localtime(&seconds);
                log_file << asctime(time_info) << " download_chunk\n";
                log_file.close();

                std::ifstream chunk_file(std::string(CS_DIRECTORY) + "/" + chunk_UUID);
                std::string result, buffer;

                while (std::getline(chunk_file, buffer)) {
                    result += buffer + "\n";
                }

                if (result.length() > 0) {
                    result.pop_back();
                }

                return result;
            }
    );

    this_CS.bind(
            "delete_chunk", [](const std::string &chunk_UUID) {
                std::ofstream log_file(std::string(CS_DIRECTORY) + "/" + "log", std::ios_base::app);
                time_t seconds = time(nullptr);
                tm* time_info = localtime(&seconds);
                log_file << asctime(time_info) << " delete_chunk\n";
                log_file.close();

                boost::filesystem::remove(std::string(CS_DIRECTORY) + "/" + chunk_UUID);
                return true;
            }
    );

    this_CS.run();

    return 0;
}
