
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

const char *progname = "check_tomcat_memory";
const char *version = "1.0.4";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name, const std::string application, const std::string type );
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *application = NULL;
char *memory_type = NULL;

int warn_percent = 90;
int crit_percent = 95;
float warn_size_bytes = 0;
float crit_size_bytes = 0;

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

  return check( server_name, application, memory_type );
}

/**
 *
 */
int check( const std::string server_name, const std::string application, const std::string type ) {

  int state = STATE_UNKNOWN;

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, application );

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
    nlohmann::json memory = "";

    if(type == "heap-mem") {
      json.find("Memory", "HeapMemoryUsage", memory);
      memory_type = "Heap";
      if(warn_percent == 0)
        warn_percent = 80;
      if(crit_percent == 0)
        crit_percent = 90;
    } else {
      json.find("Memory", "NonHeapMemoryUsage", memory);
      memory_type = "Perm";
      if(warn_percent == 0)
        warn_percent = 98;
      if(crit_percent == 0)
        crit_percent = 99;
    }

//     std::cout << memory.dump(2) << std::endl;

    int max = memory["max"];
    long used = memory["used"];
    long committed = memory["committed"];
    float percent;

    percent = 100.0 * (float)used / (float)committed;
    percent = roundf(percent * 100) / 100;

    char buf[15];
    if( max != -1 )
      memory_max_human_readable = human_readable(max, buf);

    memory_committed_human_readable = human_readable(committed, buf);
    memory_used_human_readable      = human_readable(used, buf);

    // -------------------------------------------------------------------

    if( DEBUG == true ) {

      std::cout << percent << std::endl;
      std::cout << "warn : " << warn_percent << std::endl;
      std::cout << "crit : " << crit_percent << std::endl;
    }

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

    if(type == "heap-mem") {

      std::cout
        << status << " - "
        << percent << "% " << memory_type << " Memory used"
        << "<br>"
        << "Max: " << memory_max_human_readable
        << "<br>"
        << "Commited: " << memory_committed_human_readable
        << "<br>"
        << "Used: " << memory_used_human_readable
        << " |"
        << " max=" << max
        << " committed=" << committed
        << " used=" << used
        << std::endl;
    } else {

      std::cout
        << status << " - "
        << percent << "% " << memory_type << " Memory used"
        << "<br>"
        << "Commited: " << memory_committed_human_readable
        << "<br>"
        << "Used: " << memory_used_human_readable
        << " |"
        << " committed=" << committed
        << " used=" << used
        << std::endl;
    }

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }

  return state;
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
          break;
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
      "cae-live",
      "cae-live-1",
      "cae-live-2",
      "cae-live-3",
      "cae-live-4",
      "cae-live-5",
      "cae-live-6",
      "cae-live-7",
      "cae-live-8",
      "cae-live-9"
    };

    if( !in_array( application, app )) {
      std::cerr << "'" << application << "' is no valid application!" << std::endl;
      return ERROR;
    }

  } else {
    std::cerr << "missing 'application' argument." << std::endl;
    return ERROR;
  }

  if(memory_type != NULL)  {

    std::vector<std::string> srv = { "heap-mem","perm-mem" };

    if( !in_array( memory_type, srv )) {
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
  if( warn_percent > crit_percent ) {
    std::cout << "Warning percentage (" << warn_percent << ") should be more than critical (" << crit_percent << ") percentage" << std::endl;
  } else
  if( warn_size_bytes > crit_size_bytes ) {
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
  std::cout << "valid applications are:" << std::endl;
  std::cout << "  - workflow-server, " << std::endl;
  std::cout << "  - content-feeder, " << std::endl;
  std::cout << "  - user-changes, " << std::endl;
  std::cout << "  - elastic-worker, " << std::endl;
  std::cout << "  - caefeeder-preview, " << std::endl;
  std::cout << "  - caefeeder-live, " << std::endl;
  std::cout << "  - cae-preview, " << std::endl;
  std::cout << "  - studio, " << std::endl;
  std::cout << "  - sitemanager," << std::endl;
  std::cout << "  - cae-live," << std::endl;
  std::cout << "  - cae-live-1," << std::endl;
  std::cout << "  - cae-live-2," << std::endl;
  std::cout << "  - cae-live-3," << std::endl;
  std::cout << "  - cae-live-4," << std::endl;
  std::cout << "  - cae-live-5," << std::endl;
  std::cout << "  - cae-live-6," << std::endl;
  std::cout << "  - cae-live-7," << std::endl;
  std::cout << "  - cae-live-8 and" << std::endl;
  std::cout << "  - cae-live-9" << std::endl;
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
  std::cout << " -A, --application" << std::endl;
  std::cout << "    the name of the application" << std::endl;
  std::cout << " -M, --memory" << std::endl;
  std::cout << "    the tomcat memory type." << std::endl;
  std::cout << " -w, --warning" << std::endl;
  std::cout << "    warning memory usage" << std::endl;
  std::cout << " -c, --critical" << std::endl;
  std::cout << "    critical memory usage" << std::endl;
}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " [-R <redis_server>] -H <hostname> -M <memory type>"  << std::endl;
  std::cout << std::endl;
}
