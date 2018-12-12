#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <rpc/this_handler.h>

#include "utils.hpp"


int main(int argc, const char *argv[]) {
    uint16_t port;

    if (argc != 2 || !str_to_uint16(argv[1], port)) {
        std::cout << "Usage: " << argv[0] << " port\n";
        return 1;
    }

    std::cout << "The server is running. Port: " << port << std::endl;

    std::string CS_directory(getenv("HOME"));
    CS_directory += "/plyushkincluster/servers/CS/" + std::to_string(port) + "/";

    if (boost::filesystem::create_directories(CS_directory)) {
        std::cout << "Create directory " << CS_directory << std::endl;
    }

    rpc::server this_CS(port);

    // возвращает все исключения, возникшие в забинденных ф-ях в клиент TODO: включить когда настанет время
    // this_CS.suppress_exceptions(true);

    this_CS.bind(
            "save_chunk", [=](const std::string &chunk_UUID, const std::vector<char> &chunk_content) {
                add_log(CS_directory, "save_chunk");

                std::ofstream chunk_file(CS_directory + chunk_UUID);

                for (auto const &symbol : chunk_content) {
                    chunk_file << symbol;
                }

                chunk_file.close();
            }
    );

    this_CS.bind(
            "get_chunk", [=](const std::string &chunk_UUID) {
                add_log(CS_directory, "get_chunk");

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
            "rename_chunk", [=](const std::string &old_UUID, const std::string &new_UUID) {
                add_log(CS_directory, "rename_chunk");

                boost::filesystem::rename(CS_directory + old_UUID, CS_directory + new_UUID);
            }
    );

    this_CS.bind(
            "delete_chunk", [=](const std::string &chunk_UUID) {
                add_log(CS_directory, "delete_chunk");

                boost::filesystem::remove(CS_directory + chunk_UUID);
            }
    );

    this_CS.run();

    return 0;
}