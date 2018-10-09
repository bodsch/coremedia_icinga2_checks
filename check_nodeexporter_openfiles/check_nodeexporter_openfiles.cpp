
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
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

const char *progname = "check_nodeexporter_openfiles";
const char *version = "1.0.0";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name );

void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;

float warning = 3072;
float critical = 4096;

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
  std::string status = "";

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, "node-exporter" );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    return STATE_WARNING;
  }

  if( DEBUG == true ) {
    std::cout << redis_data << std::endl;
  }

  long allocated       = 0;
  long maximum         = 0;

  try {

    Json json(redis_data);

    if( DEBUG == true ) {
      std::cout << redis_data << std::endl;
    }

    nlohmann::json filefd = "";
    json.find("filefd", filefd);

    if( !filefd.is_object() ) {
      std::cout
        << "<b>no filefd measurement points available.</b>"
        << std::endl;
      return STATE_WARNING;
    }

    if( DEBUG == true ) {
      std::cout << filefd << std::endl;
    }

    allocated = json.find("filefd", "allocated");
    maximum   = json.find("filefd", "maximum");
/*
    if(warning == 0)
      warning= 3072;

    if(critical != 4096)
      critical = maximum;
*/
    if( DEBUG == true ) {
      std::cout << allocated << std::endl;
      std::cout << maximum << std::endl;
    }

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }


  if(allocated == warning || allocated <= warning) {
    status = "OK";
    state = STATE_OK;
  } else
  if(allocated >= warning || allocated <= critical) {
    status = "WARNING";
    state = STATE_WARNING;
  } else{
    status = "CRITICAL";
    state = STATE_CRITICAL;
  }

  std::cout
    << status << " - "
    << allocated << " of " << maximum << " file descriptors open"
    << " |"
    << " open_files=" << allocated << ";" << warning << ";" << critical
    << std::endl;

  return state;
}


/*
 * process command-line arguments
 */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:Dw:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"warning"    , required_argument, nullptr, 'w'},
    {"critical"   , required_argument, nullptr, 'c'},
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
      case 'w':
        if(is_intnonneg(optarg)) {
          warning = atoi(optarg);
          break;
        } else {
          std::cout << "Warning threshold must be integer!" << std::endl;
          print_usage();
          break;
        }
      case 'c':
        if(is_intnonneg(optarg)) {
          critical = atoi(optarg);
          break;
        } else {
          std::cout <<  "Critical threshold must be integer!" << std::endl;
          print_usage();
          break;
        }

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

  if( warning > critical ) {
    std::cout << "critical should be more than warning" << std::endl;
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
  std::cout << "This plugin get the open file descriptors from the node-exporter data" << std::endl;
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
