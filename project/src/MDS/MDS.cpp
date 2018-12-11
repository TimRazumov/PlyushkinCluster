#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>
#include <string>
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/this_handler.h>

#include "MDS.h"
#include "utils.hpp"


// разделяет данные о файле и о расположении чанков, мне кажется, что жуткий костыль, так что TODO
const char *META_DATA_SEPARATOR = "~~~~";


MDS::MDS(const uint16_t &port)
    : this_server(port)
    , known_CS()
    , data(master, 2000)
    , copy_count(2)
    , turn_of_servers(0)
{
    // возвращает все исключения, возникшие в забинденных ф-ях в клиент
    this_server.suppress_exceptions(true);

    MDS_directory = getenv("HOME");
    MDS_directory += "/plyushkincluster/servers/MDS/" + std::to_string(port) + "/";

    if (boost::filesystem::create_directories(MDS_directory)) {
        std::cout << "Create directory " << MDS_directory << std::endl;
    }
    this->binding();
}

// TODO: exceptions
void MDS::binding() {
    this_server.bind(
            "is_meta_exist", [=](const std::string &file_UUID) {
                add_log(MDS_directory, "is_meta_exist");
                return boost::filesystem::exists(MDS_directory + file_UUID);
            }
    );

    // TODO: тут в вектор забивается вообще все, что есть в файле, что долго. Научите меня нормально работать с файлами, плз, тут говнокод шо песос
    this_server.bind(
            "set_attr", [=](const std::string &file_UUID, const std::vector<std::string> &attr) {
                if (!boost::filesystem::exists(MDS_directory + file_UUID)) {
                    add_log(MDS_directory, "set_attr (new file)");

                    std::ofstream meta_data_out(MDS_directory + file_UUID);
                    for (const auto &field : attr) {
                        meta_data_out << field << " ";
                    }

                    meta_data_out << std::string(META_DATA_SEPARATOR) << " ";

                    meta_data_out.close();

                    return;
                }

                add_log(MDS_directory, "set_attr");

                std::ifstream meta_data_in(MDS_directory + file_UUID);
                if (!meta_data_in) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
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

                std::ofstream meta_data_out(MDS_directory + file_UUID);
                for (const auto &field : attr) {
                    meta_data_out << field << " ";
                }

                meta_data_out << std::string(META_DATA_SEPARATOR);

                for (const auto &field : chunk_data) {
                    meta_data_out << field << " ";
                }

                meta_data_out.close();
            }
    );

    this_server.bind(
            "get_attr", [=](const std::string &file_UUID) {
                add_log(MDS_directory, "get_attr");

                std::ifstream meta_data(MDS_directory + file_UUID);
                if (!meta_data) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::vector<std::string> attr;
                std::string buffer;

                while (!meta_data.eof()) {
                    meta_data >> buffer;
                    if (buffer == META_DATA_SEPARATOR) {
                        break;
                    }
                    attr.push_back(buffer);
                }

                meta_data.close();

                rpc::this_handler().respond(attr);
            }
    );

    this_server.bind(
            "save_chunk", [=](const std::string &file_UUID, const size_t &chunk_num, const std::vector<char> &chunk_content) {
                add_log(MDS_directory, "save_chunk");

                if (known_CS.empty()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::ifstream meta_data_in(MDS_directory + file_UUID);
                if (!meta_data_in) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                while (!meta_data_in.eof()) {
                    std::string buffer;
                    meta_data_in >> buffer;
                    if (buffer == META_DATA_SEPARATOR) {
                        break;
                    }
                }

                std::vector<size_t> CS_with_chunk;
                while (true) {
                    size_t num_in_file;

                    // считать элемент, отвечающий за номер чанка
                    meta_data_in >> num_in_file;

                    if (meta_data_in.eof()) {
                        break;
                    }

                    if (num_in_file == chunk_num) {
                        // запомнить номера серверов, где хранится данный чанк
                        for (size_t i = 0; i < copy_count; ++i) {
                            meta_data_in >> num_in_file;
                            CS_with_chunk.push_back(num_in_file);
                        }
                    } else {
                        // скипнуть ненужную информацию
                        for (size_t i = 0; i < copy_count; ++i) {
                            meta_data_in >> num_in_file;
                        }
                    }
                }

                meta_data_in.close();

                if (CS_with_chunk.empty()) {
                    // действия в случае, если чанка с таким номером не существует
//                    std::vector<rpc::client> CS;
                    std::ofstream meta_data_out(MDS_directory + file_UUID, std::ios_base::app);

                    meta_data_out << chunk_num;

                    for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                        rpc::client CS(known_CS[turn_of_servers].get_info().addr, known_CS[turn_of_servers].get_info().port);
                        CS.set_timeout(data.get_info().timeout);
                        CS.send("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);
//                        CS.emplace_back(known_CS[turn_of_servers].get_info().addr, known_CS[turn_of_servers].get_info().port);
//                        CS.back().set_timeout(data.get_info().timeout);
//                        CS.back().send("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);

                        meta_data_out << turn_of_servers;

                        turn_of_servers = (turn_of_servers + 1) % known_CS.size();
                    }

                    meta_data_out.close();
                } else {
                    // действия в случае, если чанк с таким номером существует
//                    std::vector<rpc::client> CS;

                    for (auto const &i : CS_with_chunk) {
                        rpc::client CS(known_CS[i].get_info().addr, known_CS[i].get_info().port);
                        CS.set_timeout(data.get_info().timeout);
                        CS.send("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);
//                        CS.emplace_back(known_CS[i].get_info().addr, known_CS[i].get_info().port);
//                        CS.back().set_timeout(data.get_info().timeout);
//                        CS.back().send("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);
                    }
                }
            }
    );

    this_server.bind(
            "get_chunk", [=](const std::string &file_UUID, const size_t &chunk_num) {
                add_log(MDS_directory, "get_chunk");

                if (known_CS.empty()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::ifstream meta_data_in(MDS_directory + file_UUID);
                if (!meta_data_in) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                while (!meta_data_in.eof()) {
                    std::string buffer;
                    meta_data_in >> buffer;
                    if (buffer == META_DATA_SEPARATOR) {
                        break;
                    }
                }

                size_t num_in_file;

                // считать элемент, отвечающий за номер чанка
                meta_data_in >> num_in_file;

                if (meta_data_in.eof()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                // взять номер первого сервера, где хранится данный чанк
                meta_data_in >> num_in_file;

                if (meta_data_in.eof()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                meta_data_in.close();

                rpc::client CS(known_CS[num_in_file].get_info().addr, known_CS[num_in_file].get_info().port);

                return CS.call("get_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num))).as<std::vector<char>>();
            }
    );
    // TODO: rename
    this_server.bind(
            "rename", [=](const std::string &old_file_UUID, const std::string &new_file_UUID) {

            }
    );
    // TODO: delete(file_UUID)
}

void MDS::add_CS(const std::string &addr, uint16_t port) {
    known_CS.emplace_back(CS_data(addr, port));
}

// TODO: должны менять на всех МДС
void MDS::change_timeout(int64_t new_timeout) {
    data.change_timeout(new_timeout);
}

void MDS::change_status() {
    data.change_status();
}