
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

const char *progname = "check_nodeexporter_memory";
const char *version = "0.0.1";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name );

void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;

int warn_percent = 0;
int crit_percent = 0;
int warning = 0;
int critical = 0;

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

  if(warning == 0)
    warning= 50;

  if(critical == 0)
    critical= 20;

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, "node-exporter" );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    return STATE_WARNING;
  }

  std::string status = "";

  std::string mem_available = "0";
  std::string mem_free      = "0";
  std::string mem_total     = "0";
  float mem_used         = 0;
  float mem_used_percent = 0;

  std::string swap_total  = "0";
  std::string swap_cached = "0";
  std::string swap_free   = "0";
  float swap_used         = 0;
  float swap_used_percent = 0;

  std::string mem_total_human_readable = "";
  std::string mem_used_human_readable = "";
  std::string swap_total_human_readable = "";
  std::string swap_used_human_readable = "";

  std::ostringstream stringStream;
  std::string output_status;

  char buf[10];

  try {

    Json json(redis_data);

    nlohmann::json memory = "";
    json.find("memory", memory);

    if( memory.is_null() )    { std::cout << "is null" << std::endl; }
    if( memory.is_boolean() ) { std::cout << "is boolean" << std::endl; }
    if( memory.is_number() )  { std::cout << "is number" << std::endl; }
    if( memory.is_object() )  { std::cout << "is object" << std::endl; }
    if( memory.is_array() )   { std::cout << "is array" << std::endl; }
    if( memory.is_string() )  { std::cout << "is string" << std::endl; }

    if( !memory.is_object() ) {
      std::cout
        << "<b>no memory measurement points available.</b>"
        << std::endl;
      return STATE_WARNING;
    }

    std::cout << memory.dump(2) << std::endl;

    json.find("memory", "MemTotal", mem_total);
    json.find("memory", "MemFree", mem_free);
    json.find("memory", "MemAvailable", mem_available);

    json.find("memory", "SwapTotal", swap_total);
    json.find("memory", "SwapFree", swap_free);
    json.find("memory", "SwapCached", swap_cached);

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }

// -----------------------------

  mem_used          = std::stof(mem_total.c_str()) - std::stof(mem_available.c_str());
  mem_used_percent  = ( 100 * mem_used / std::stof(mem_total.c_str()) );
  mem_used_percent  = roundf(mem_used_percent *100) / 100;

  std::cout << "mem_total " << mem_total << std::endl;
  std::cout << "mem_free  " << mem_free << std::endl;
  std::cout << "mem_avail " << mem_available << std::endl;

  std::cout << "mem_used " << mem_used << std::endl;
  std::cout << "mem_used % " << mem_used_percent << std::endl;

  if( std::stol(swap_total.c_str()) != 0 ) {

    swap_used          = std::stof(swap_total.c_str()) - std::stof(swap_free.c_str());
    swap_used_percent  = 100 * swap_used / std::stof(swap_total.c_str());
    swap_used_percent  = roundf(swap_used_percent *100) / 100;

    if( swap_used_percent == warning || swap_used_percent <= warning ) {
      status    = "OK";
      state = STATE_OK;
    } else
    if( swap_used_percent >= warning && swap_used_percent <= critical ) {
      status    = "WARNING";
      state = STATE_WARNING;
    } else {
      status    = "CRITICAL";
      state = STATE_CRITICAL;
    }

    std::cout << "swap_total " << std::stof(swap_total.c_str()) << std::endl;
    std::cout << "swap_free  " << std::stof(swap_free.c_str()) << std::endl;

    std::cout << "swap_used  "  << swap_used << std::endl;
    std::cout << "swap_used % " << swap_used_percent << std::endl;

    if( state != STATE_OK ) {
      stringStream << "(" << status << ")";
      std::string output_status = stringStream.str();
    }

    swap_total_human_readable = human_readable(std::stol(swap_total.c_str()), buf);
    swap_used_human_readable = human_readable(swap_used, buf);

    std::cout
      << "swap  - "
      << "size: " << swap_total_human_readable << ", "
      << "used: " << swap_used_human_readable << ", "
      << "used percent: " << swap_used_percent << "%"
      << output_status
      << "<br>";
  }

  if( mem_used_percent == warning || mem_used_percent <= warning ) {
    status    = "OK";
    state = STATE_OK;
  } else
  if( mem_used_percent >= warning && mem_used_percent <= critical ) {
    status    = "WARNING";
    state = STATE_WARNING;
  } else {
    status    = "CRITICAL";
    state = STATE_CRITICAL;
  }

  if( state != STATE_OK ) {
    stringStream.clear();
    stringStream << "(" << status << ")";
    std::string output_status = stringStream.str();

//     std::string output_status = "(" << status << ")";
  }

  mem_total_human_readable = human_readable(std::stol(mem_total.c_str()), buf);
  mem_used_human_readable = human_readable(mem_used, buf);

  std::cout
    << "memory  - "
    << "size: " << mem_total_human_readable << ", "
    << "used: " << mem_used_human_readable << ", "
    << "used percent: " << mem_used_percent << "%"
    << output_status
    << std::endl;

  return state;
}


/*
 * process command-line arguments
 */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:w:c:";
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
      case 'w':                  /* warning size threshold */
        if(is_intnonneg(optarg)) {
          warning = atoi(optarg);
          break;
        } else {
          std::cout << "Warning threshold must be integer!" << std::endl;
          print_usage();
        }
      case 'c':                  /* critical size threshold */
        if(is_intnonneg(optarg)) {
          critical = atoi(optarg);
          break;
        } else {
          std::cout <<  "Critical threshold must be integer!" << std::endl;
          print_usage();
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
  std::cout << "This plugin get the load value from the node-exporter data" << std::endl;
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
