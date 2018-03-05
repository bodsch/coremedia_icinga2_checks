
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

const char *progname = "check_memory";
const char *version = "0.0.1";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name, const std::string application, const std::string type );
bool in_array(const std::string &value, const std::vector<std::string> &array);
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *application = NULL;
char *memory_type = NULL;


int warn_percent = 0;
int crit_percent = 0;
float warn_size_bytes = 0;
float crit_size_bytes = 0;


/**
 *
 */
int main(int argc, char **argv) {

  int result = STATE_UNKNOWN;

  if( process_arguments(argc, argv) == ERROR ) {
//     std::cout << "Could not parse arguments"  << std::endl;
    std::cout << std::endl;
    print_usage();
    return STATE_UNKNOWN;
  }

  result = check( server_name, application, memory_type );

  return result;
}

/**
 *
 */
int check( const std::string server_name, const std::string application, const std::string type ) {

  int state = STATE_UNKNOWN;

  Redis r(redis_server, 6379);
  std::string cache_key = r.cache_key( server_name, application );

  std::cout << cache_key << std::endl;

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    std::cout << "WARNING - no data in our data store found."  << std::endl;
    return STATE_WARNING;
  }

  try {

    Json json(redis_data);
    nlohmann::json memory = "";

    if(type == "heap-mem") {
      json.find("Memory", "HeapMemoryUsage", memory);
    } else {
      json.find("Memory", "NonHeapMemoryUsage", memory);
    }

//     std::cout << memory.dump(2) << std::endl;

    int max = memory["max"];
    uint used = memory["used"];
    uint committed = memory["committed"];
    float percent;

    percent = 100.0 * (float)used / (float)committed;
    percent = roundf(percent * 100) / 100;

    // -------------------------------------------------------------------

    std::cout << warn_percent << std::endl;
    std::cout << crit_percent << std::endl;

    std::string status= "";

    if( percent == warn_percent || percent <= warn_percent ) {
      status    = "OK";
      state = STATE_OK;
    } else
    if( percent >= warn_percent && percent <= crit_percent ) {
      status    = "WARNING";
      state = STATE_WARNING;
    } else {
      status    = "CRITICAL";
      state = STATE_CRITICAL;
    }

    std::cout << status << std::endl;

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }


  return state;
}




/*
 * process command-line arguments
 */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:A:M:w:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"application", required_argument, nullptr, 'A'},
    {"memory"     , required_argument, nullptr, 'M'},
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
      case 'A':
        application = optarg;
        break;
      case 'M':
        memory_type = optarg;
        break;
      case 'w':                  /* warning size threshold */
        if(is_intnonneg (optarg)) {
          warn_size_bytes = (float) atoi (optarg);
          break;
        } else
        if( strstr (optarg, ",") && strstr (optarg, "%") && sscanf (optarg, "%f,%d%%", &warn_size_bytes, &warn_percent) == 2) {
          warn_size_bytes = floorf(warn_size_bytes);
          break;
        } else
        if (strstr (optarg, "%") && sscanf (optarg, "%d%%", &warn_percent) == 1) {
          break;
        } else {
          std::cout << "Warning threshold must be integer or percentage!" << std::endl;
          print_usage();
        }
      case 'c':                  /* critical size threshold */
        if(is_intnonneg (optarg)) {
          crit_size_bytes = (float) atoi (optarg);
          break;
        } else
        if (strstr (optarg, ",") && strstr (optarg, "%") && sscanf (optarg, "%f,%d%%", &crit_size_bytes, &crit_percent) == 2) {
          crit_size_bytes = floorf(crit_size_bytes);
          break;
        } else
        if (strstr (optarg, "%") && sscanf (optarg, "%d%%", &crit_percent) == 1) {
          break;
        } else {
          std::cout <<  "Critical threshold must be integer or percentage!" << std::endl;
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

  if(application != NULL)  {

    std::vector<std::string> app = {
      "content-management-server",
      "master-live-server",
      "replication-live-server",
      "workflow-server",
      "content-feeder",
      "user-changes",
      "elastic-worker",
      "caefeeder-preview",
      "caefeeder-live",
      "cae-preview",
      "studio",
      "sitemanager",
      "cae-live"
    };

    if( in_array( application, app )) {
      return OK;
    } else {
      std::cerr << "'" << application << "' is no valid application!" << std::endl;
      return ERROR;
    }

  } else {
    std::cerr << "missing 'application' argument." << std::endl;
    return ERROR;
  }

  if(memory_type != NULL)  {

    std::vector<std::string> srv = { "heap-mem","perm-mem" };

    if( in_array( memory_type, srv )) {
      return OK;
    } else {
      std::cerr << "'" << memory_type << "' is no valid memory type!" << std::endl;
      return ERROR;
    }

  } else {
    std::cerr << "missing 'memory' argument." << std::endl;
    return ERROR;
  }

  if( warn_percent == 0 && crit_percent == 0 && warn_size_bytes == 0 && crit_size_bytes == 0 ) {
    return ERROR;
  } else
  if( warn_percent < crit_percent ) {
    std::cout << "Warning percentage should be more than critical percentage" << std::endl;
  } else
  if( warn_size_bytes < crit_size_bytes ) {
    std::cout << "Warning free space should be more than critical free space" << std::endl;
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
  std::cout << "This plugin will return the used tomcat memory corresponding to the content server" << std::endl;
  std::cout << "valid memory types are:" << std::endl;
  std::cout << "  - heap-mem and " << std::endl;
  std::cout << "  - perm-mem" << std::endl;
  print_usage();
  std::cout << "Options:" << std::endl;
  std::cout << " -h, --help" << std::endl;
  std::cout << "    Print detailed help screen" << std::endl;
  std::cout << " -V, --version" << std::endl;
  std::cout << "    Print version information" << std::endl;

  std::cout << " -R, --redis" << std::endl;
  std::cout << "    the redis service who stored the measurements data." << std::endl;
  std::cout << " -H, --hostname" << std::endl;
  std::cout << "    the host to be checked." << std::endl;
  std::cout << " -A, --application" << std::endl;
  std::cout << "    the name of the application" << std::endl;
  std::cout << " -M, --memory" << std::endl;
  std::cout << "    the tomcat memory type." << std::endl;

}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " -R <redis_server> -H <hostname> -M <memory type>"  << std::endl;
  std::cout << std::endl;
}

/**
 *
 */
bool in_array(const std::string &value, const std::vector<std::string> &array) {
  return std::find(array.begin(), array.end(), value) != array.end();
}
