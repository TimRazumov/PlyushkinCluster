#ifndef PROJRCT_INCLUDE_UTILS_H
#define PROJRCT_INCLUDE_UTILS_H

// stat
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h>

// uuid
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp> 
#include <boost/uuid/uuid_io.hpp>

// str_to_uint16
#include <inttypes.h>

#include <string>
#include <iostream>
#include <fstream>
#include <bitset>

const std::vector<unsigned int> perms_t = {
    0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007
};

std::string uuid_from_str(std::string const &path);
int min(int a, int b);
std::vector<char>::iterator line_end(std::vector<char>::iterator it);
std::vector<std::string> attrs_to_string(std::vector<unsigned int> attrs);
std::string getDirByPath(const std::string &path);
std::string nameByPath(const std::string &path);
std::vector<unsigned int> get_perm(mode_t mode);

void add_log(const std::string &directory, const std::string &log);
bool str_to_uint16(const char *str, uint16_t &res);

#endif
