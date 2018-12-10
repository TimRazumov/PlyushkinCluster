#ifndef PLYUSHKINCLUSTER_KNOWN_CS_H
#define PLYUSHKINCLUSTER_KNOWN_CS_H


#include <iostream>

typedef struct {
    std::string addr;
    uint16_t port = 0;
} CS_info;


class CS_data {
private:
    CS_info info;
public:
    CS_data(const std::string &addr, uint16_t port) : info{addr, port} {}
    ~CS_data() = default;
    CS_info get_info() const { return info; }
};


#endif //PLYUSHKINCLUSTER_KNOWN_CS_H
