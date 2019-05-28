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


MDS::MDS(const uint16_t &port)
    : this_server(port)
    , known_CS()
    , data(master, 2000)
    , copy_count(2)
    , turn_of_servers(0)
    , my_zk_clt(zk::client::connect("zk://127.0.0.1:2182").get())
{
    if (!exists_node("/CLUSTER")) {
        create_empty_node("/CLUSTER", zk::create_mode::normal);
        // my_zk_clt.set("/CLUSTER", str_to_vec_ch("id:0")); // TODO: когда кластер станет одной сущностью следует писать его id
        create_empty_node("/CLUSTER/MDS", zk::create_mode::normal);
        create_empty_node("/CLUSTER/RENOVATION", zk::create_mode::normal);
        create_empty_node("/CLUSTER/CS", zk::create_mode::normal);
        create_empty_node("/CLUSTER/META", zk::create_mode::normal);
    }

    create_empty_node("/CLUSTER/MDS/", zk::create_mode::ephemeral | zk::create_mode::sequential);

    // возвращает все исключения, возникшие в забинденных ф-ях в клиент TODO: включить когда настанет время
    // this_server.suppress_exceptions(true);

    MDS_directory = getenv("HOME");
    MDS_directory += "/plyushkincluster/servers/MDS/" + std::to_string(port) + "/";

    if (boost::filesystem::create_directories(MDS_directory)) {
        std::cout << "Create directory " << MDS_directory << std::endl;
    }

    binding();
}


void MDS::create_empty_node(const std::string &dir, zk::create_mode node_type) {
    my_zk_clt.create(dir, {}, node_type);
    my_zk_clt.set(dir, {});
}

bool MDS::exists_node(const std::string &path) {
    return static_cast<bool>(my_zk_clt.exists(path).get());
}

// TODO: exceptions
void MDS::binding() {
    const std::string META_ZK_DIR = "/CLUSTER/META/";
    const std::string GLOBAL_CS_PATH = "/CLUSTER/CS";
    this_server.bind(
        "is_meta_exist", [=](const std::string &file_uuid) {
            add_log(MDS_directory, "is_meta_exist");
            return exists_node(META_ZK_DIR + file_uuid);
        }
    );
    this_server.bind(
        "set_attr", [=](const std::string &file_uuid, const std::vector<std::string> &attr) {
            const std::string meta_path = META_ZK_DIR + file_uuid;

            if (exists_node(meta_path)) {
                add_log(MDS_directory, "set_attr: " + file_uuid);
            } else {
                add_log(MDS_directory, "set_attr (new node): " + file_uuid);
                create_empty_node(meta_path, zk::create_mode::normal);
                create_empty_node(meta_path + "/chunk_locations", zk::create_mode::normal);
            }

            auto info_old_buff = my_zk_clt.get(meta_path).get().data();

            nlohmann::json info_json;
                info_json = info_old_buff.size() != 0
                        ? nlohmann::json::parse(my_zk_clt.get(meta_path).get().data())
                        : MetaEntityInfo::get_empty_json();

            auto meta_entity_info = MetaEntityInfo(info_json);
            meta_entity_info.set_attr(const_cast<std::vector<std::string> &>(attr));

            auto buff = meta_entity_info.to_json().dump();

            my_zk_clt.set(meta_path, zk::buffer(buff.begin(), buff.end()));
        }
    );

    this_server.bind(
        "get_attr", [=](const std::string &file_uuid) {
            add_log(MDS_directory, "get_attr");
            std::vector<std::string> attr;
            const std::string meta_path = META_ZK_DIR + file_uuid;
            if (exists_node(meta_path)) {
                auto info_buff = my_zk_clt.get(meta_path).get().data();
                auto info_json = nlohmann::json::parse(info_buff);

                attr = std::move(MetaEntityInfo(info_json).get_attr());

                std::cout << "gotcha in get attr____:" << info_buff.data() << std::endl;//TODO
                std::cout << "metapath____:" << meta_path << std::endl; //TODO
            } else {
                std::cout << "gotcha error in get attr______________: " << meta_path << std::endl;//TODO
                rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
            }
            rpc::this_handler().respond(attr);
        }
    );

    this_server.bind(
            "save_chunk", [=](const std::string &file_UUID, const size_t &chunk_num, const std::vector<char> &chunk_content) {
                add_log(MDS_directory, "save_chunk");
                std::cout << "gotcha ___" << chunk_num << "___" << std::endl;//TODO
                const std::string meta_path = META_ZK_DIR + file_UUID;
                const std::string chunks_locations_dir = meta_path + "/chunk_locations/";

                if (!exists_node(meta_path)) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                auto concrete_meta_data = my_zk_clt.get(meta_path).get();
                auto global_cs_data = my_zk_clt.get(GLOBAL_CS_PATH).get();

                if (global_cs_data.stat().children_count == 0) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                    std::cout << "###### no CS  #####" << std::endl;//TODO
                }
                std::cout << "____go ahead_save_chunk_____" << chunk_num << std::endl;//TODO

                auto meta_entity_info = MetaEntityInfo( // получаем инфу метаноды
                        nlohmann::json::parse(concrete_meta_data.data())
                        );

                auto chunk_entity_info = ChunkEntityInfo( // получаем инфу о расположениях чанка
                        exists_node(chunks_locations_dir + std::to_string(chunk_num)) // сущ ли вообще нода
                        ? nlohmann::json::parse(
                                my_zk_clt.get(chunks_locations_dir + std::to_string(chunk_num)).get().data()
                                )
                        : ChunkEntityInfo::get_empty_json()
                     );


                if (chunk_entity_info.get_locations().empty()) {
                    std::cout << "__chunk_not_exists__" << chunk_num << std::endl;//TODO
                    // действия в случае, если чанка с таким номером не существует
                    auto chunk_servers = my_zk_clt.get_children(GLOBAL_CS_PATH).get(); // все доступные CS

                    // количество копий сравниваем с доступными серверами и даём миимальное
                    auto replica_count = std::min<size_t>(copy_count, chunk_servers.children().size());

                    for (size_t i = 0; i < replica_count; ++i) { // тут закидывваем чанки
                        add_log(MDS_directory, "-new chunk");
                        {
                            const auto cs_id = chunk_servers.children()[turn_of_servers]; // получайм cs_id

                            meta_entity_info.get_on_cs().insert(std::stoul(cs_id)); // обновляем on_cs у метаноды
                            // добавляем cs_id в locations ноды расположения
                            chunk_entity_info.get_locations().insert(std::stoul(cs_id));


                            auto concrete_cs_data = ConcreteCsEntityInfo( // получаем data выбранного cs и парсим
                                    nlohmann::json::parse(
                                            my_zk_clt.get(GLOBAL_CS_PATH + "/" + cs_id).get().data()
                                            )
                                    );

                            rpc::client CS(concrete_cs_data.get_ip(), concrete_cs_data.get_port());

                            CS.set_timeout(data.get_info().timeout); // хз, потом просто константу сделаем что б выпилить
                            CS.call("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);
                        }

                        turn_of_servers = (turn_of_servers + 1) % chunk_servers.children().size();

                        std::cout << "_____________success set new chunk_____________" << std::endl;
                    }

                    auto locations_dump = chunk_entity_info.to_json().dump(); // сериализуем locations
                    my_zk_clt.create(
                            chunks_locations_dir + std::to_string(chunk_num),  // создаём ноду локаций конкретного чанка
                            zk::buffer(locations_dump.begin(), locations_dump.end())
                            );

                    auto meta_dump = meta_entity_info.to_json().dump(); // сериализуем метаинфу
                    my_zk_clt.set(
                            meta_path,
                            zk::buffer(meta_dump.begin(), meta_dump.end()) // обновляем метаинфу (точнее on_cs)
                            );

                } else {
                    // действия в случае, если чанк с таким номером существует
                    std::cout << "__chunk_exists__" << chunk_num << std::endl;//TODO
                    for (auto const &cs_id : chunk_entity_info.get_locations()) {
                        add_log(MDS_directory, "-update chunk");
                        {
                            auto concrete_cs_data = ConcreteCsEntityInfo( // получаем data выбранного cs и парсим
                                    nlohmann::json::parse(
                                            my_zk_clt.get(GLOBAL_CS_PATH + "/" + std::to_string(cs_id))
                                            .get().data()
                                    )
                            );


                            rpc::client CS(concrete_cs_data.get_ip(), concrete_cs_data.get_port());

                            CS.set_timeout(data.get_info().timeout);

                            std::cout << "_____________success set existed chunk_____________" << std::endl;

                            CS.call("save_chunk", uuid_from_str(file_UUID + std::to_string(chunk_num)), chunk_content);
                        }
                    }
                }
            }
    );

    this_server.bind(
            "get_chunk", [=](const std::string &file_UUID, const size_t &chunk_num) {
                add_log(MDS_directory, "get_chunk");
                std::cout << "gotcha___" << file_UUID <<
                    "______get_chunk____" << chunk_num << "________" << std::endl;//TODO

                const std::string meta_path = META_ZK_DIR + file_UUID;
                const std::string chunks_locations_dir = meta_path + "/chunk_locations/";
                const std::string chunk_record_path = chunks_locations_dir + std::to_string(chunk_num);

                if (!exists_node(meta_path)) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                }

                auto concrete_meta_data = my_zk_clt.get(meta_path).get();
                auto global_cs_data = my_zk_clt.get(GLOBAL_CS_PATH).get();

                if (global_cs_data.stat().children_count == 0) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                    std::cout << "###############  no CS in get chunk  ###############" << std::endl;//TODO
                }

                if (!exists_node(chunk_record_path)) {
                    rpc::this_handler().respond_error(std::make_tuple(1, "UNKNOWN ERROR"));
                    std::cout << "######  can't find meta record in get chunk  ####"
                        << std::endl;//TODO
                }

                auto chunk_locations_data = my_zk_clt.get(chunk_record_path).get().data();
                auto chunk_entity_info = ChunkEntityInfo(nlohmann::json::parse(chunk_locations_data));


                const auto first_cs = *(chunk_entity_info.get_locations().begin());
                auto concrete_cs_data = my_zk_clt.get(
                        GLOBAL_CS_PATH + "/" + std::to_string(first_cs)
                        ).get().data();

                auto concrete_cs_entity_info = ConcreteCsEntityInfo(nlohmann::json::parse(concrete_cs_data));


                rpc::client CS(concrete_cs_entity_info.get_ip(), concrete_cs_entity_info.get_port());

                std::cout << "_____________success get chunk_____________" << std::endl;

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
