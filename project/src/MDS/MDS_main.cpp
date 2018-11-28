#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include "rpc/server.h"
#include "rpc/client.h"

#include "MDS.h"

//TODO first week
//Клиент-сервер  // ля, а какие? у рпц так-то реализованы
//Protobuf, select  // не нужно
//Передача чанков в/из хранилища  // ez
//Хранение файлов в хранилище  // ez

int main() {
    if (boost::filesystem::create_directories(MDS_DIRECTORY)) {
        std::cout << "mkdir " << MDS_DIRECTORY << std::endl;
    }

    rpc::server this_MDS(5000);

    rpc::client CS("127.0.0.1", 8080);
    CS.set_timeout(default_timeout);

    this_MDS.bind(
            "stop_server", [&this_MDS]() {
                this_MDS.stop();
                return true;
            }
    );

    this_MDS.bind(
            "set_CS_timeout", [&CS](unsigned int new_timeout) {
                CS.set_timeout(new_timeout);
                return true;
            }
    );

    this_MDS.bind(
            "save_chunk", [&CS](std::string const &chunk_UUID, std::string const &chunk_content) {
                return CS.call("save_chunk", test_chunk_UUID, chunk_content).as<bool>();
            }
    );

    this_MDS.bind(
            "download_chunk", [&CS](std::string const &chunk_UUID, std::string const &chunk_content) {
                return CS.call("download_chunk", test_chunk_UUID).as<std::string>();
            }
    );

    this_MDS.run();

    return 0;
}
