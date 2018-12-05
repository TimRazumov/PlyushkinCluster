#ifndef PLYUSHKINCLUSTER_KNOWN_MDS_H
#define PLYUSHKINCLUSTER_KNOWN_MDS_H

# include <stdint.h>

typedef enum {slave, master} MDS_status;

// все данные об MDS сервере, которые могут понадобиться остальным кускам кода
typedef struct {
    // сервер master/slave
    MDS_status status;

    // ms
    int64_t timeout;
} MDS_info;


class MDS_data {
private:
    MDS_info info;
public:
    MDS_info get_info() { return info; }

    void change_timeout(int64_t new_timeout) { info.timeout = new_timeout; }
};


#endif //PLYUSHKINCLUSTER_KNOWN_MDS_H
