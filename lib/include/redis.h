
#ifndef REDIS_H
#define REDIS_H

#include <string>
#include <cctype>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>

// #include <common.h>
#include "md5.h"
#include "common.h"
#include <redox.hpp>

class Redis {

  public:
                      Redis(const std::string s, int p = 6379) { _server = s; _port = p; }

    std::string       cache_key(const std::string hostname, const std::string service);
    bool              get(const std::string key, std::string &result);


  private:
    std::string _server;
    int _port;
};

#endif // REDIS_H
