#include "utils.hpp"

int min(int a, int b) {
    return a < b ? a : b;
}

std::vector<char>::iterator line_end(std::vector<char>::iterator it) {
    while (*it != '\n')
        it++;
    return it;
}

std::vector<std::string> attrs_to_string(std::vector<unsigned int> attrs) {
    std::vector<std::string> set_attrs;
    for (const auto &attr : attrs) {
        set_attrs.push_back(std::to_string(attr));
    }
    return set_attrs;
}

std::string getDirByPath(const std::string& path) {
    int bias = 0;
    auto iter = path.end() - 1;
    while (*iter != '/') {
        bias++;
        iter--;
    }
    auto ret = path.substr(0, path.size() - bias - 1);
    if (ret.empty()) {
        return "/";
    }
    return ret;
}

std::string nameByPath(const std::string& path) {
    int bias = 0;
    auto iter = path.end() - 1;
    while (*iter != '/') {
        bias++;
        iter--;
    }
    auto ret = std::string(iter+1, path.end());
    return ret;
}

std::string uuid_from_str(std::string const &path) {
    boost::uuids::nil_generator gen;
    boost::uuids::name_generator path_gen(gen());
    return to_string(path_gen(path));
}

void add_log(const std::string &directory, const std::string &log) {
    std::ofstream log_file(directory + "log", std::ios_base::app);
    time_t seconds = time(nullptr);
    tm* time_info = localtime(&seconds);
    log_file << asctime(time_info) << " " << log << "\n";
    log_file.close();
}

bool str_to_uint16(const char *str, uint16_t &res) {
    char *end;
    errno = 0;
    intmax_t val = strtoimax(str, &end, 10);
    if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0')
        return false;
    res = (uint16_t) val;
    return true;
}
