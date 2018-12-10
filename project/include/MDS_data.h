#ifndef PLYUSHKINCLUSTER_KNOWN_MDS_H
#define PLYUSHKINCLUSTER_KNOWN_MDS_H

# include <stdint.h>

enum MDS_status {slave, master};

// все данные об MDS сервере, которые могут понадобиться остальным кускам кода
struct MDS_info {
    // сервер master/slave
    MDS_status status;

    // ms
    int64_t timeout;

    MDS_info(const MDS_status &status, const int64_t &timeout) : status(status), timeout(timeout) {}
};


class MDS_data {
private:
    MDS_info info;
public:
    MDS_data(const MDS_status &status, const int64_t &timeout) : info(status, timeout) {}
    MDS_info const get_info() { return info; }

    void change_timeout(const int64_t &new_timeout) { info.timeout = new_timeout; }
    void change_status() { info.status == master ? info.status = slave : info.status = master; }
};


#endif //PLYUSHKINCLUSTER_KNOWN_MDS_H
