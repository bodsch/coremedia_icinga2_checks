
#include "../include/json.h"


/**
 * search an json object from type nlohmann::json
 */
void Json::find(const std::string base, nlohmann::json &result) {

  nlohmann::json j = string2json(_json);
  result = json_value(j, base);
}

/**
 * search an json object from type string
 */
void Json::find(const std::string base, const std::string search, std::string &result) {

  nlohmann::json j = string2json(_json);
  nlohmann::json v = json_value(j, base);

  try {
    result = v[search];
  } catch(...) {
    result = "";
  }
}

/**
 * search an json object from type int
 */
void Json::find(const std::string base, const std::string search, int &result) {

  nlohmann::json j = string2json(_json);
  nlohmann::json v = json_value(j, base);

  try {
    result = v[search];
  } catch(...) {
    result = -1;
  }
}

/**
 * search an json object from type long
 */
void Json::find(const std::string base, const std::string search, long &result) {

  nlohmann::json j = string2json(_json);
  nlohmann::json v = json_value(j, base);

  try {
    result = v[search];
  } catch(...) {
    result = -1;
  }
}

/**
 * search an json object from type float
 */
void Json::find(const std::string base, const std::string search, float &result) {

  nlohmann::json j = string2json(_json);
  nlohmann::json v = json_value(j, base);

  try {
    result = v[search];
  } catch(...) {
    result = -1;
  }
}

/**
 * search an json object from type nlohmann::json
 */
void Json::find(const std::string base, const std::string search, nlohmann::json &result) {

  nlohmann::json j = string2json(_json);
  nlohmann::json v = json_value(j, base);

  try {
    result = v[search];
  } catch(...) {
    result = -1;
  }
}

/**
 * search an json object from type bool
 */
void Json::find(const std::string base, const std::string search, bool &result) {

  nlohmann::json j = string2json(_json);
  nlohmann::json v = json_value(j, base);

  try {
    result = v[search];
  } catch(...) {
    result = -1;
  }
}

/**
 * search an json object
 */
nlohmann::json Json::find(const std::string base, const std::string search) {

  nlohmann::json j = string2json(_json);
  nlohmann::json v = json_value(j, base);

  nlohmann::json r;
  try {
    r = v[search];
  } catch(...) {
    r = -1;
  }
  return r;
}


/**
 * extract the 'value'part from our json object
 */
nlohmann::json Json::json_value(nlohmann::json j, std::string s) {

  int size = j.size();
  nlohmann::json search;
  nlohmann::json value;

  if( j.is_object() ) {
    auto it_find_value  = j.find(s);

    if(it_find_value != j.end())
      value = j[s];
  } else {
    for( int i = 0; i <= size; ++i) {
      search = j.at(i);
      if ( search.find(s) != search.end()) { break; }
    }

    if( search.size() == 1 ) {

      try {
        value = search[s]["value"];
        // get am iterator to the first element
        nlohmann::json::iterator it = value.begin();

        if(it.key().find("com.coremedia") != std::string::npos) {
          value = value[it.key()];
        }
      } catch (nlohmann::json::parse_error& e) {
        // output exception information
        std::cout << "message: " << e.what() << '\n'
                  << "exception id: " << e.id << '\n'
                  << "byte position of error: " << e.byte << std::endl;
      } catch(...) {
        std::cout << "ERROR - can\'t parse json."  << std::endl;
      }
    }
  }
  return value;
}

/**
 * transform a string to a json object
 */
nlohmann::json Json::string2json(std::string string) {

  // replace some ruby specialist things
  //
  replaceAll(string, "=>", ":");
  replaceAll(string, "nil", "\"\"");

  nlohmann::json j;

  try {
    j = nlohmann::json::parse(string);

  } catch (nlohmann::json::parse_error& e) {
    // output exception information
    std::cout << "message: " << e.what() << '\n'
              << "exception id: " << e.id << '\n'
              << "byte position of error: " << e.byte << std::endl;
  } catch(...) {
    std::cout << "ERROR - can\'t parse json."  << std::endl;
  }

  return j;
}


/**
 * helper funktion for replacments
 */
void Json::replaceAll(std::string& str, const std::string& from, const std::string& to) {

  if(from.empty())
    return;

  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

/**
 * helper function to dump an json object
 */
void Json::dump() {

  nlohmann::json j = string2json(_json);
  for (auto it = j.begin(); it != j.end(); ++it) {
    std::cout << it.key() << " | " << it.value() << std::endl;
  }
}

