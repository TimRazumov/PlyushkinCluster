#ifndef PROJRCT_INCLUDE_UTILS_H
#define PROJRCT_INCLUDE_UTILS_H

// stat
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h>

//uuid
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp> 
#include <boost/uuid/uuid_io.hpp>

#include <string>
#include <iostream>

std::string uuid_from_str(std::string const &path);


#endif
