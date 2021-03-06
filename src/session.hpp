#ifndef SESSION_HPP
#define SESSION_HPP

#include "path.hpp"
#include <stdint.h>
#include <map>

namespace Session {
    extern std::map<uint32_t, path_t> paths;
    int new_path();
    int writeBson(path_t* path);
    int loadBson();
    int removeBson(uint32_t id);
}

#endif