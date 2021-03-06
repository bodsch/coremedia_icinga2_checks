
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
const char *version = "1.1.1";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name );

void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;

int warn_percent = 85;
int crit_percent = 95;
float warning = 0;
float critical = 0;

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

  long mem_total        = 0;
  long mem_used         = 0;
  long mem_used_percent = 0;

  long swap_total        = 0;
  long swap_used         = 0;
  long swap_used_percent = 0;

  std::string mem_total_human_readable = "";
  std::string mem_used_human_readable = "";
  std::string swap_total_human_readable = "";
  std::string swap_used_human_readable = "";

  std::ostringstream stringStream;
  std::string output_status;

  char buf[10];

  try {

    Json json(redis_data);

    if( DEBUG == true ) {
      std::cout << redis_data << std::endl;
    }

    nlohmann::json memory = "";
    nlohmann::json swap = "";
    json.find("memory", memory);

    if( !memory.is_object() ) {
      std::cout
        << "<b>no memory measurement points available.</b>"
        << std::endl;
      return STATE_WARNING;
    }

    if( DEBUG == true ) {
      std::cout << memory << std::endl;
    }
    /*
     * {"available":2077310976,"free":159703040,"total":10316976128,"used":8239665152,"used_percent":79}
     * {"cached":35016704,"free":4179431424,"total":4294963200,"used":115531776,"used_percent":2}
     */
    memory = json.find("memory", "memory");
    swap   = json.find("memory", "swap");

    if( DEBUG == true ) {
      std::cout << memory << std::endl;
      std::cout << swap << std::endl;
    }
    // {"available":1107382272,"free":138268672,"total":2985177088,"used":1877794816,"used_percent":62}
    mem_total        = memory["total"];
    mem_used         = memory["used"];
    mem_used_percent = memory["used_percent"];

    // {"cached":0,"free":0,"total":0}
    swap_total        = swap["total"];
    if( swap_total != 0) {
      swap_used         = swap["used"];
      swap_used_percent = swap["used_percent"];
    }
  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }

  if( swap_total != 0 ) {

    if( swap_used_percent == warn_percent || swap_used_percent <= warn_percent ) {
      status    = "OK";
      state = STATE_OK;
    } else
    if( swap_used_percent >= warn_percent && swap_used_percent <= crit_percent ) {
      status    = "WARNING";
      state = STATE_WARNING;
    } else {
      status    = "CRITICAL";
      state = STATE_CRITICAL;
    }

    if( state != STATE_OK ) {
      stringStream << "(" << status << ")";
      output_status = stringStream.str();
    }

    swap_total_human_readable = human_readable(swap_total, buf);
    swap_used_human_readable = human_readable(swap_used, buf);

    std::cout
      << "swap    - "
      << "size: " << swap_total_human_readable << ", "
      << "used: " << swap_used_human_readable << ", "
      << "used percent: " << swap_used_percent << "% "
      << output_status
      << "<br>";
  }

  if( mem_used_percent == warn_percent || mem_used_percent <= warn_percent ) {
    status    = "OK";
    state = STATE_OK;
  } else
  if( mem_used_percent >= warn_percent && mem_used_percent <= crit_percent ) {
    status    = "WARNING";
    state = STATE_WARNING;
  } else {
    status    = "CRITICAL";
    state = STATE_CRITICAL;
  }

  if( state != STATE_OK ) {
    stringStream.clear();
    stringStream << "(" << status << ")";
    output_status = stringStream.str();
  }

  mem_total_human_readable = human_readable(mem_total, buf);
  mem_used_human_readable = human_readable(mem_used, buf);

  std::cout
    << "memory  - "
    << "size: " << mem_total_human_readable << ", "
    << "used: " << mem_used_human_readable << ", "
    << "used percent: " << mem_used_percent << "% "
    << output_status
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
      case 'w':                  /* warning size threshold */
        if (is_intnonneg (optarg)) {
          warning = (float) atoi (optarg);
          break;
        }
        else if (strstr (optarg, ",") &&
                 strstr (optarg, "%") &&
                 sscanf (optarg, "%f,%d%%", &warning, &warn_percent) == 2) {
          warning = floorf(warning);
          break;
        }
        else if (strstr (optarg, "%") &&
                 sscanf (optarg, "%d%%", &warn_percent) == 1) {
          break;
        }
        else {
          std::cout << "Warning threshold must be integer or percentage!" << std::endl;
          break;
        }
      case 'c':                  /* critical size threshold */
        if (is_intnonneg (optarg)) {
          critical = (float) atoi (optarg);
          break;
        }
        else if (strstr (optarg, ",") &&
                 strstr (optarg, "%") &&
                 sscanf (optarg, "%f,%d%%", &critical, &crit_percent) == 2) {
          critical = floorf(critical);
          break;
        }
        else if (strstr (optarg, "%") &&
                 sscanf (optarg, "%d%%", &crit_percent) == 1) {
          break;
        }
        else {
          std::cout << "Critical threshold must be integer or percentage!" << std::endl;
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
