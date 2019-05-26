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

static const char META_SEPARATOR = '~';

static zk::server::configuration create_zk_config()
{
    zk::server::configuration conf = zk::server::configuration::make_minimal("zk-data", 2181)
                  .add_server(1, "192.168.1.101")
                  .add_server(2, "192.168.1.102")
                  .add_server(3, "192.168.1.103");
    conf.save_file("settings.cfg");
    return conf;
}

static std::vector<char> str_to_vec_ch(const std::string& str)
{
    return std::vector<char>(str.begin(), str.end());
}

static std::vector<char> vec_str_to_vec_ch(const std::vector<std::string>& vec_str)
{
    std::vector<char> vec_ch;
    for (int i = 0, size = vec_str.size(); i < size; i++) {
        std::vector<char> tmp = str_to_vec_ch(vec_str[i]);
        vec_ch.insert(vec_ch.end(), tmp.begin(), tmp.end());
        if (i != size - 1) {
            vec_ch.push_back(META_SEPARATOR);
        }    
    }
    return vec_ch;
}

static std::vector<std::string> vec_ch_to_vec_str(const std::vector<char>& vec_ch)
{
    std::vector<std::string> vec_str;
    std::string tmp;
    for (const char &ch : vec_ch) {
        if (ch == META_SEPARATOR) {
            vec_str.push_back(tmp);
            tmp.clear();
        } else {
            tmp += ch;
        }
    }
    return vec_str;
}

MDS::MDS(const uint16_t &port)
    : this_server(port)
    , known_CS()
    , data(master, 2000)
    , copy_count(2)
    , turn_of_servers(0)
    , my_zk_srvr(create_zk_config())
    , my_zk_clt(zk::client::connect("zk://127.0.0.1:2181").get())
{
    create_mode("/CLUSTER", zk::create_mode::normal);
    // my_zk_clt.set("/CLUSTER", str_to_vec_ch("id:0")); // TODO: когда кластер станет одной сущностью следует писать его id
    create_mode("/CLUSTER/MDS", zk::create_mode::normal);
    create_mode("/CLUSTER/CS", zk::create_mode::normal);
    create_mode("/CLUSTER/META", zk::create_mode::normal);

    // возвращает все исключения, возникшие в забинденных ф-ях в клиент TODO: включить когда настанет время
    // this_server.suppress_exceptions(true);

    MDS_directory = getenv("HOME");
    MDS_directory += "/plyushkincluster/servers/MDS/" + std::to_string(port) + "/";

    if (boost::filesystem::create_directories(MDS_directory)) {
        std::cout << "Create directory " << MDS_directory << std::endl;
    }
    binding();
}


void MDS::create_mode(const std::string& dir, zk::create_mode mode_type) {
    my_zk_clt.create(dir, {}, mode_type);
    my_zk_clt.set(dir, {}); // патамушта
}

bool MDS::exists_mode(const std::string& path) {
    try {
        my_zk_clt.get(path).get();
        return true;
    } catch (...) {
        return false;
    }
}

// TODO: exceptions
void MDS::binding() {
    const std::string meta_zk_dir = "/CLUSTER/META/";
    this_server.bind(
        "is_meta_exist", [=](const std::string &file_uuid) {
            add_log(MDS_directory, "is_meta_exist");
            return exists_mode(meta_zk_dir + file_uuid);
        }
    );
    this_server.bind(
        "set_attr", [=](const std::string &file_uuid, const std::vector<std::string> &attr) {
            const std::string dir = meta_zk_dir + file_uuid;
            if (exists_mode(dir)) {
                add_log(MDS_directory, "set_attr (new node)");
                create_mode(dir, zk::create_mode::normal);
                std::ofstream file_chunk(dir);
            } else {
                add_log(MDS_directory, "set_attr");
            }
            my_zk_clt.set(dir, vec_str_to_vec_ch(attr));
            return;
        }
    );

    this_server.bind(
        "get_attr", [=](const std::string &file_uuid) {
            add_log(MDS_directory, "get_attr");
            std::vector<char> attr;
            const std::string dir = meta_zk_dir + file_uuid;
            if (exists_mode(dir)) {
                attr = my_zk_clt.get(meta_zk_dir + file_uuid).get().data();
            } else {
                rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
            }
            rpc::this_handler().respond(vec_ch_to_vec_str(attr));
            return;
        }
    );

    this_server.bind(
            "save_chunk", [=](const std::string &file_UUID, const size_t &chunk_num, const std::vector<char> &chunk_content) {
                add_log(MDS_directory, "save_chunk");

                if (known_CS.empty()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::ifstream file_chunk(MDS_directory + file_UUID);
                if (!file_chunk) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::vector<size_t> CS_with_chunk;
                while (true) {
                    size_t num_in_file;

                    // считать элемент, отвечающий за номер чанка
                    file_chunk >> num_in_file;

                    if (file_chunk.eof()) {
                        break;
                    }

                    if (num_in_file == chunk_num) {
                        // запомнить номера серверов, где хранится данный чанк
                        for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                            file_chunk >> num_in_file;
                            CS_with_chunk.push_back(num_in_file);
                        }
                        break;
                    } else {
                        // скипнуть ненужную информацию
                        for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                            file_chunk >> num_in_file;
                        }
                    }
                }

                file_chunk.close();

                if (CS_with_chunk.empty()) {
                    // действия в случае, если чанка с таким номером не существует
                    std::ofstream meta_data_out(MDS_directory + file_UUID, std::ios_base::app);

                    meta_data_out << chunk_num << " ";

                    for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                        add_log(MDS_directory, "-new chunk");
                        {
                            rpc::client CS(known_CS[turn_of_servers].get_info().addr,
                                           known_CS[turn_of_servers].get_info().port);
                            CS.set_timeout(data.get_info().timeout);
                            CS.call("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);
                        }
                        meta_data_out << turn_of_servers << " ";

                        turn_of_servers = (turn_of_servers + 1) % known_CS.size();
                    }

                    meta_data_out.close();
                } else {
                    // действия в случае, если чанк с таким номером существует
                    for (auto const &i : CS_with_chunk) {
                        add_log(MDS_directory, "-update chunk");
                        {
                            rpc::client CS(known_CS[i].get_info().addr, known_CS[i].get_info().port);
                            CS.set_timeout(data.get_info().timeout);
                            CS.call("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);
                        }
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

                std::ifstream file_chunk(MDS_directory + file_UUID);
                if (!file_chunk) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                size_t num_in_file;

                // считать элемент, отвечающий за номер чанка
                file_chunk >> num_in_file;

                if (file_chunk.eof()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                // взять номер первого сервера, где хранится данный чанк
                file_chunk >> num_in_file;

                if (file_chunk.eof()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                file_chunk.close();

                rpc::client CS(known_CS[num_in_file].get_info().addr, known_CS[num_in_file].get_info().port);

                return CS.call("get_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num))).as<std::vector<char>>();
            }
    );

    this_server.bind(
            "rename_file", [=](const std::string &old_file_UUID, const std::string &new_file_UUID) {
                add_log(MDS_directory, "rename_file");

                if (known_CS.empty()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::ifstream file_chunk(MDS_directory + old_file_UUID);
                if (!file_chunk) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                while (true) {
                    std::vector<size_t> CS_with_chunk;
                    size_t chunk_num;
                    size_t server_num;

                    file_chunk >> chunk_num;

                    if (file_chunk.eof()) {
                        break;
                    }

                    for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                        file_chunk >> server_num;
                        CS_with_chunk.push_back(server_num);
                    }

                    for (const auto &srv : CS_with_chunk) {
                        {
                            rpc::client CS(known_CS[srv].get_info().addr, known_CS[srv].get_info().port);
                            CS.set_timeout(data.get_info().timeout);
                            CS.call("rename_chunk", uuid_from_str(old_file_UUID + std::to_string(chunk_num))
                                                  , uuid_from_str(new_file_UUID + std::to_string(chunk_num)));
                        }
                    }

                    CS_with_chunk.clear();
                }

                file_chunk.close();

                boost::filesystem::rename(MDS_directory + old_file_UUID, MDS_directory + new_file_UUID);
            }
    );

    this_server.bind(
            "delete_chunk", [=](const std::string &file_UUID, const size_t &chunk_num) {
                add_log(MDS_directory, "delete_chunk");

                if (known_CS.empty()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::ifstream file_chunk(MDS_directory + file_UUID);
                if (!file_chunk) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                std::vector<size_t> CS_with_chunk;
                while (true) {
                    size_t num_in_file;

                    // считать элемент, отвечающий за номер чанка
                    file_chunk >> num_in_file;

                    if (file_chunk.eof()) {
                        break;
                    }

                    if (num_in_file == chunk_num) {
                        // запомнить номера серверов, где хранится данный чанк
                        for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                            file_chunk >> num_in_file;
                            CS_with_chunk.push_back(num_in_file);
                        }
                        break;
                    } else {
                        // скипнуть ненужную информацию
                        for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                            file_chunk >> num_in_file;
                        }
                    }
                }

                file_chunk.close();

                if (CS_with_chunk.empty()) {
                    // действия в случае, если чанка с таким номером не существует
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                } else {
                    // действия в случае, если чанк с таким номером существует
                    for (auto const &i : CS_with_chunk) {
                        {
                            rpc::client CS(known_CS[i].get_info().addr, known_CS[i].get_info().port);
                            CS.set_timeout(data.get_info().timeout);
                            CS.call("delete_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)));
                        }
                    }
                }
            }
    );

    this_server.bind(
            "delete_file", [=](const std::string &file_UUID) {
                add_log(MDS_directory, "delete_file");
                if (known_CS.empty()) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }
                std::ifstream file_chunk(MDS_directory + file_UUID);
                if (!file_chunk) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }
                while (true) {
                    std::vector<size_t> CS_with_chunk;
                    size_t chunk_num;
                    size_t server_num;
                    file_chunk >> chunk_num;
                    if (file_chunk.eof()) {
                        break;
                    }
                    for (size_t i = 0; i < std::min<size_t>(copy_count, known_CS.size()); ++i) {
                        file_chunk >> server_num;
                        CS_with_chunk.push_back(server_num);
                    }
                    for (const auto &srv : CS_with_chunk) {
                        {
                            rpc::client CS(known_CS[srv].get_info().addr, known_CS[srv].get_info().port);
                            CS.set_timeout(data.get_info().timeout);
                            CS.call("delete_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)));
                        }
                    }
                    CS_with_chunk.clear();
                }
                file_chunk.close();
                boost::filesystem::remove(MDS_directory + file_UUID);
            }
    );
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
