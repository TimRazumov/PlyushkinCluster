#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <ctime>


// TODO: безопасность

int main() {
    std::string CS_directory(getenv("HOME"));
    CS_directory += "/plyushkincluster/servers/CS/";

    if (boost::filesystem::create_directories(CS_directory)) {
        std::cout << "Create directory " << CS_directory << std::endl;
    }

    rpc::server this_CS(5100);

    this_CS.bind(
            "save_chunk", [=](const std::string &chunk_UUID, const std::vector<char> &chunk_content) {
                std::ofstream log_file(CS_directory + "log", std::ios_base::app);
                time_t seconds = time(nullptr);
                tm* time_info = localtime(&seconds);
                log_file << asctime(time_info) << " save_chunk\n";
                log_file.close();


                std::ofstream chunk_file(CS_directory + chunk_UUID);

                for (auto const &symbol : chunk_content) {
                    chunk_file << symbol;
                }

                chunk_file.close();
            }
    );

    this_CS.bind(
            "get_chunk", [=](const std::string &chunk_UUID) {
                std::ofstream log_file(CS_directory + "log", std::ios_base::app);
                time_t seconds = time(nullptr);
                tm* time_info = localtime(&seconds);
                log_file << asctime(time_info) << " get_chunk\n";
                log_file.close();

                std::ifstream chunk_file(CS_directory + chunk_UUID);
                std::vector<char> chunk_content;
                char buffer;

                chunk_file.get(buffer);
                while (!chunk_file.eof()) {
                    chunk_content.push_back(buffer);
                    chunk_file.get(buffer);
                };

                chunk_file.close();
                return chunk_content;
            }
    );

    this_CS.bind(
            "delete_chunk", [=](const std::string &chunk_UUID) {
                std::ofstream log_file(CS_directory + "log", std::ios_base::app);
                time_t seconds = time(nullptr);
                tm* time_info = localtime(&seconds);
                log_file << asctime(time_info) << " delete_chunk\n";
                log_file.close();

                boost::filesystem::remove(CS_directory + chunk_UUID);
            }
    );

    this_CS.run();

    return 0;
}