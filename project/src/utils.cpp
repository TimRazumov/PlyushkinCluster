#include "utils.hpp"

std::string uuid_from_str(std::string const &path) {
    boost::uuids::nil_generator gen;
    boost::uuids::name_generator path_gen(gen());
    return to_string(path_gen(path));
}
