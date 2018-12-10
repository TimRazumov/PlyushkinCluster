#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <rpc/this_handler.h>
#include <ctime>
#include <inttypes.h>


// TODO: move add_log and str_to_uint16 to utils

void add_log(const std::string &directory, const std::string &log) {
    std::ofstream log_file(directory + "log", std::ios_base::app);
    time_t seconds = time(nullptr);
    tm* time_info = localtime(&seconds);
    log_file << asctime(time_info) << " " << log << "\n";
    log_file.close();
}

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

    std::cout << "The server is running. Port: " << port << std::endl;

    std::string CS_directory(getenv("HOME"));
    CS_directory += "/plyushkincluster/servers/CS/" + std::to_string(port) + "/";

    if (boost::filesystem::create_directories(CS_directory)) {
        std::cout << "Create directory " << CS_directory << std::endl;
    }

    rpc::server this_CS(5100);

    // возвращает все исключения, возникшие в забинденных ф-ях в клиент
    this_CS.suppress_exceptions(true);

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
            "delete_chunk", [=](const std::string &chunk_UUID) {
                add_log(CS_directory, "delete_chunk");

                boost::filesystem::remove(CS_directory + chunk_UUID);
            }
    );

    this_CS.run();

    return 0;
}