#include "utils.hpp"

int min(int a, int b) {
    return a < b ? a : b;
}

std::vector<char>::iterator line_end(std::vector<char>::iterator it) {
    while (*it != '\n')
        it++;
    return it;
}

std::vector<std::string> attrs_to_string(std::vector<int> attrs) {
    std::vector<std::string> set_attrs;
    for (int i = 0; i < attrs.size(); i++) {
        set_attrs.push_back(std::to_string(attrs[i]));
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
    return path.substr(0, path.size() - bias - 1);
}

std::string uuid_from_str(std::string const &path) {
    boost::uuids::nil_generator gen;
    boost::uuids::name_generator path_gen(gen());
    return to_string(path_gen(path));
}
