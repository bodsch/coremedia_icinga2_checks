
#ifndef JSON_H
#define JSON_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>

#include <common.h>

#include <nlohmann/json.hpp>

class Json {

  public:
                      Json(const std::string s ) { _json = s; }

    void              find(const std::string base, const std::string search, std::string &result);
    void              find(const std::string base, const std::string search, int &result);
    nlohmann::json    find(const std::string base, const std::string search);

  private:

    nlohmann::json  json_value(nlohmann::json j, std::string s);
    nlohmann::json  string2json(std::string string);
    void            dump(nlohmann::json j);

    void            replaceAll(std::string& str, const std::string& from, const std::string& to);

    std::string _json;
};

#endif // JSON_H
