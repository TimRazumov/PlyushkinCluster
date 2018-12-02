#ifndef PLYUSHKINCLUSTER_MDS_H
#define PLYUSHKINCLUSTER_MDS_H


#include <vector>
#include <rpc/server.h>

#include "MDS_data.h"
#include "CS_data.h"

// TODO: перенести куски кода в .cpp файл
// TODO: translate comments
// TODO: проверка, не отвалились ли серваки (отделно MDS и CS)
// TODO: действия в случае отключения сервака
// TODO: разные действия сервака в зависимости от его статуса (master/slave)

// основной класс для демона, работающего на серверах метаданных
class MDS {
private:
    // я хз, шо тут комментировать. Крч, основа класса. Не стал делать наследником по причине "ну нахер"
    rpc::server this_server;

    // поддержание связи с другими серваками
    std::vector<MDS_data> known_MDS;
    std::vector<CS_data> known_CS;

    // information
    MDS_data data;

    // задел на будущую супер систему безопасности
    std::string password;

    // номер сервера, на который будет класться следующий чанк
    // скорее всего, временная хрень, ибо лучше выбирать сервак в зависимости от его заполненности
    std::size_t turn_of_servers;

public:
    // all .bind inside the constructor
    explicit MDS(uint16_t port);
    ~MDS() = default;

    void async_run(const std::size_t &worker_threads = 1) { this_server.async_run(worker_threads); }
    void stop() { this_server.stop(); }

    std::vector<MDS_data> get_known_MDS() { return known_MDS; }
    std::vector<CS_data> get_known_CS() { return known_CS; }

    void add_CS(std::string const &addr, uint16_t port);
};


#endif //PLYUSHKINCLUSTER_MDS_H
