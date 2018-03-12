
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

const char *progname = "check_runlevel";
const char *version = "1.0.1";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name, const std::string content_server );
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *content_server = NULL;

/**
 *
 */
int main(int argc, char **argv) {

  if( process_arguments(argc, argv) == ERROR ) {
    std::cout << std::endl;
    print_usage();
    return STATE_UNKNOWN;
  }

  return check( server_name, content_server );
}

/**
 *
 */
int check( const std::string server_name, const std::string content_server ) {

  int state = STATE_UNKNOWN;

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, content_server );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    std::cout << "WARNING - no data in our data store found."  << std::endl;
    return STATE_WARNING;
  }

  try {

    Json json(redis_data);

    std::string runlevel = "";
    int runlevel_numeric = 0;
    json.find("Server", "RunLevel", runlevel);
    json.find("Server", "RunLevelNumeric", runlevel_numeric);

    std::string status = "";

    if( runlevel_numeric == -1 ) {

      std::transform(runlevel.begin(), runlevel.end(), runlevel.begin(), ::tolower);

      if(runlevel == "offline") { status = "CRITICAL"; state  = STATE_CRITICAL; }
      else
      if( runlevel == "online") { status = "OK"; state  = STATE_OK; }
      else
      if( runlevel == "administration") { status = "WARNING"; state  = STATE_WARNING; }
      else { status = "CRITICAL"; state  = STATE_CRITICAL; }
    } else {

      if( runlevel_numeric == 0) { runlevel = "stopped"; status = "WARNING";  state  = STATE_WARNING; }
      else
      if( runlevel_numeric == 1) { runlevel = "starting"; status = "UNKNOWN";  state  = STATE_UNKNOWN; }
      else
      if( runlevel_numeric == 2) { runlevel = "initializing"; status = "UNKNOWN";  state  = STATE_UNKNOWN; }
      else
      if( runlevel_numeric == 3) { runlevel = "running"; status = "OK";  state  = STATE_OK; }
      else { runlevel = "failed"; status = "CRITICAL";  state  = STATE_CRITICAL; }
    }

    std::cout
      << status << " - "
      << "Runlevel in <b>" << runlevel << "</b> Mode" << std::endl;

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }

  return(state);
}


/*
 * process command-line arguments
 */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:C:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"contentserver", required_argument, nullptr, 'C'},
    {nullptr      , 0, nullptr, 0}
  };

  if(argc < 2)
    return ERROR;

  int long_index =0;
  while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1) {

    switch (opt) {
      case 'h':
        print_help();
        exit(STATE_UNKNOWN);
      case 'V':
        std::cout << progname << " v" << version << std::endl;
        exit(STATE_UNKNOWN);
      case 'R':
        redis_server = optarg;
        break;
      case 'H':
        server_name = optarg;
        break;
      case 'C':
        content_server = optarg;
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

  if(content_server != NULL)  {

    std::vector<std::string> app = {
      "content-management-server",
      "master-live-server",
      "replication-live-server"
    };

    if( in_array( content_server, app )) {
      return OK;
    } else {
      std::cerr << "'" << content_server << "' is no valid content server!" << std::endl;
      return ERROR;
    }

  } else {
    std::cerr << "missing 'contentserver' argument." << std::endl;
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
  std::cout << "This plugin will return the runlevel state corresponding to the content server" << std::endl;
  std::cout << "valid content server are:" << std::endl;
  std::cout << "  - content-management-server, " << std::endl;
  std::cout << "  - master-live-server and " << std::endl;
  std::cout << "  - replication-live-server" << std::endl;
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
  std::cout << " -C, --contentserver" << std::endl;
  std::cout << "    the content server." << std::endl;

}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " [-R <redis_server>] -H <hostname> -C <content server>"  << std::endl;
  std::cout << std::endl;
}
