
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <regex>
#include <cctype>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <getopt.h>

#include <common.h>
#include <resolver.h>
#include <md5.h>

#include <redis.h>
#include <json.h>

const char *progname = "check_publisher";
const char *version = "1.0.0";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name );
std::string parse_ior( const std::string url );
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;

bool DEBUG = false;

/**
 *
 */
int main(int argc, char **argv) {

  if( process_arguments(argc, argv) == ERROR ) {
    std::cout << std::endl;
    print_usage();
    return STATE_UNKNOWN;
  }

  return check( server_name );
}

/**
 *
 */
int check( const std::string server_name ) {

  int state = STATE_UNKNOWN;

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, "content-management-server" );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    std::cout << "WARNING - no data in our data store found."  << std::endl;
    return STATE_WARNING;
  }

  if( DEBUG == true ) {
    std::cout << redis_data << std::endl;
  }

  try {

    std::string memory_type = "";
    std::string memory_max_human_readable = "";
    std::string memory_committed_human_readable = "";
    std::string memory_used_human_readable = "";
    std::string status = "";

    Json json(redis_data);

    if( DEBUG == true ) {
      nlohmann::json publisher = "";
      json.find("Publisher", publisher);
      std::cout << publisher.dump(2) << std::endl;
    }

    bool connected = false;
    std::string last_publication_result = "";
    std::string ior_url  = "";
    std::string hostname = "";

    json.find("Publisher", "Connected", connected);
    json.find("Publisher", "LastPublResult", last_publication_result);
    json.find("Publisher", "IorUrl", ior_url);

    if( ior_url != "" ) {
      hostname = parse_ior(ior_url);
    }

    if( last_publication_result == "" ) {
      last_publication_result = "unknown";
    }

    if( DEBUG == true ) {
      std::cout << "connected        : " << connected << std::endl;
      std::cout << "last publ result : " << last_publication_result << std::endl;
      std::cout << "ior              : " << ior_url << std::endl;
      std::cout << "hostname         : " << hostname << std::endl;
    }

    if( connected == true ) {

      std::cout
        << "OK - "
        << "connected to MLS: " << hostname
        << " - last publication result: " << last_publication_result
        << std::endl;

      state  = STATE_OK;
    } else {
      std::cout
        << "CRITICAL - not connected to MLS: " << hostname << std::endl;

      state  = STATE_CRITICAL;
    }

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    state = STATE_WARNING;
  }

  return state;
}

std::string parse_ior( const std::string url ) {

  std::string result;
  unsigned counter = 0;

  std::regex url_regex (
    R"(^(([^:\/?#]+):)?(//([^\/?#]*))?([^?#]*):([?0-9].+)(#(.*))?)",
    std::regex::extended
  );
  std::smatch url_match_result;

  if( DEBUG == true ) {
    std::cout << "Checking: " << url << std::endl;

    if (std::regex_match(url, url_match_result, url_regex)) {

      std::cout << url_match_result.size() << std::endl;
      for (const auto& res : url_match_result) {
        std::cout << counter++ << ": " << res << std::endl;
      }

      result = url_match_result[4];
    } else {

      std::cerr << "Malformed url." << std::endl;
    }
  }

  if(std::regex_match(url, url_match_result, url_regex) && url_match_result.size() >= 4) {

    result = url_match_result[4];

  }

  return result;
}

/*
 * process command-line arguments
 */
int process_arguments(int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:DA:M:w:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {nullptr      , 0, nullptr, 0}
  };

  if(argc < 2)
    return ERROR;

  int long_index =0;
  while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1) {

    switch (opt) {
      case 'h':
        print_help();
        exit(STATE_OK);
      case 'V':
        std::cout << progname << " v" << version << std::endl;
        exit(STATE_OK);
      case 'R':
        redis_server = optarg;
        break;
      case 'H':
        server_name = optarg;
        break;
      case 'D':
        DEBUG = true;
        break;

      default:

        print_usage();
        exit(STATE_UNKNOWN);
    }
  }
  return validate_arguments();
}

/**
 *
 */
int validate_arguments(void) {

  Resolver resolver;

  if(redis_server == NULL)
    redis_server = std::getenv("REDIS_HOST");

  if(redis_server != NULL) {

    if(resolver.is_host(redis_server) == false) {
      std::cout << progname << " Invalid redis host or address " << optarg << std::endl;
      return ERROR;
    }
  } else {
    std::cerr << "missing 'redis' argument." << std::endl;
    return ERROR;
  }

  if(server_name != NULL) {

    if(resolver.is_host(server_name) == false) {
      std::cout << progname << " Invalid hostname or address " << optarg << std::endl;
      return ERROR;
    }
  } else {
    std::cerr << "missing 'hostname' argument." << std::endl;
    return ERROR;
  }

  return OK;
}

/**
 *
 */
void print_help (void) {

  std::cout << std::endl;
  std::cout << progname << " v" << version << std::endl;
  std::cout << "  Copyright (c) " << copyright << " " << email << std::endl;
  std::cout << std::endl;
  std::cout << "This plugin will check the publisher connection " << std::endl;

  print_usage();
  std::cout << "Options:" << std::endl;
  std::cout << " -h, --help" << std::endl;
  std::cout << "    Print detailed help screen" << std::endl;
  std::cout << " -V, --version" << std::endl;
  std::cout << "    Print version information" << std::endl;

  std::cout << " -R, --redis" << std::endl;
  std::cout << "    (optional) the redis service who stored the measurements data." << std::endl;
  std::cout << " -H, --hostname" << std::endl;
  std::cout << "    the host to be checked." << std::endl;
}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " [-R <redis_server>] -H <hostname>"  << std::endl;
  std::cout << std::endl;
}
