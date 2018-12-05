#include "MDS.h"
#include <iostream>
#include <fstream>
#include <rpc/client.h>


// разделяет данные о файле и о расположении чанков, мне кажется, что жуткий костыль, так что TODO
const char *META_DATA_SEPARATOR = "~~~~";


MDS::MDS(const uint16_t &port)
    : this_server(port)
    , known_MDS()
    , known_CS()
    , data()
    , password("password")
    , turn_of_servers(0)
{
// TODO: замени этот чанк, пришли следующий
// TODO: добавь чанк
// TODO: возвращать вектор метаданных по запросу на чтение директории
// TODO: создание нового пустого файла
// TODO: delete file
// TODO: rename
// TODO: выровнять по ширине шоб красиво было


// TODO: тут в вектор забивается вообще все, что есть в файле, что долго. Научите меня нормально работать с файлами, плз, тут говнокод шо песос
    this_server.bind(
            "set_attr", [](const std::string &file_UUID, const std::vector<std::string> &attr) {
                std::ifstream meta_data_in(file_UUID);
                if (!meta_data_in) {
                    return false;
                }

                std::vector<std::string> chunk_data;
                std::string buffer;

                while (!meta_data_in.eof()) {
                    meta_data_in >> buffer;
                    if (buffer == META_DATA_SEPARATOR) {
                        break;
                    }
                }

                while (!meta_data_in.eof()) {
                    meta_data_in >> buffer;
                    chunk_data.push_back(buffer);
                }

                meta_data_in.close();

                std::ofstream meta_data_out(file_UUID);
                for (const auto &field : attr) {
                    meta_data_out << field;
                }

                meta_data_out << std::string(META_DATA_SEPARATOR);

                for (const auto &field : chunk_data) {
                    meta_data_out << field;
                }

                meta_data_out.close();

                return true;
            }
    );

    this_server.bind(
            "get_attr", [](const std::string &file_UUID) {
                std::ifstream meta_data(file_UUID);
                if (!meta_data) {
                    return std::vector<std::string>();
                }

                std::vector<std::string> attr;
                std::string a;

                while (!meta_data.eof()) {
                    meta_data >> a;
                    if (a == META_DATA_SEPARATOR) {
                        break;
                    }
                    attr.push_back(a);
                }

                meta_data.close();

                return attr;
            }
    );

    // TODO: добавить ф-ю uuid
    // TODO: херня)) нужно для произвольного кол-ва копий
    this_server.bind(
            "save_chunk", [=](const std::string &file_UUID, const size_t chunk_num, const std::string &chunk_content) {
                if (known_CS.empty()) {
                    return false;
                }
                rpc::client CS(known_CS[turn_of_servers].get_info().addr,known_CS[turn_of_servers].get_info().port);
                CS.set_timeout(data.get_info().timeout);
                CS.call("save_chunk", file_UUID + std::to_string(chunk_num)/*uuid(file_UUID, chunk_num)*/, chunk_content).as<bool>();

                if (known_CS.size() > 1) {
                    turn_of_servers = (turn_of_servers + 1) % known_CS.size();

                    rpc::client second_CS(known_CS[turn_of_servers].get_info().addr, known_CS[turn_of_servers].get_info().port);
                    second_CS.set_timeout(data.get_info().timeout);
                    second_CS.call("save_chunk", file_UUID + std::to_string(chunk_num)/*uuid(file_UUID, chunk_num)*/, chunk_content).as<bool>();

                    turn_of_servers = (turn_of_servers + 1) % known_CS.size();
                }

                return true;
            }
    );

    // TODO: just do it
//    this_server.bind(
//            "download_chunk", [](const std::string &chunk_UUID, const std::string &chunk_content) {
//                return CS.call("download_chunk", chunk_UUID).as<std::string>();
//            }
//    );
}

void MDS::add_CS(const std::string &addr, uint16_t port) {
    known_CS.emplace_back(CS_data(addr, port));
}

void MDS::change_timeout(int64_t new_timeout) {
    data.change_timeout(new_timeout);
}