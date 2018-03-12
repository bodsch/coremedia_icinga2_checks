
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

    long              timestamp(const std::string base);
    int               status(const std::string base);

    void              find(const std::string base, nlohmann::json &result);
    void              find(const std::string base, const std::string search, std::string &result);
    void              find(const std::string base, const std::string search, int &result);
    void              find(const std::string base, const std::string search, long &result);
    void              find(const std::string base, const std::string search, float &result);
    void              find(const std::string base, const std::string search, nlohmann::json &result);
    void              find(const std::string base, const std::string search, bool &result);
    nlohmann::json    find(const std::string base, const std::string search);

    void              dump();
  private:

    nlohmann::json  json_value(nlohmann::json j, std::string s);
    nlohmann::json  string2json(std::string string);


    void            replaceAll(std::string& str, const std::string& from, const std::string& to);

    std::string _json;
};

#endif // JSON_H
