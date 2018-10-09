
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

const char *progname = "check_tomcat_openfiles";
const char *version = "1.0.0";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name, const std::string application );
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *application = NULL;

int warning = 128;
int critical = 256;

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

  return check( server_name, application );
}

/**
 *
 */
int check( const std::string server_name, const std::string application ) {

  int state = STATE_UNKNOWN;
  std::string status = "";
  int open_file_descr_count = 0;
  int max_file_descr_count = 0;
  float system_cpu_load = 0.0;

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, application );

  if( DEBUG == true ) {
    std::cout << "server_name " << server_name << std::endl;
    std::cout << "application " << application << std::endl;
    std::cout << "cache_key   " << cache_key << std::endl;
  }

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    std::cout << "WARNING - no data in our data store found."  << std::endl;
    return STATE_WARNING;
  }
/*
  if( DEBUG == true ) {
    std::cout << redis_data << std::endl;
  }
*/

  try {

    Json json(redis_data);
    nlohmann::json operating_system = "";

    json.find("OperatingSystem", "OpenFileDescriptorCount", open_file_descr_count);
    json.find("OperatingSystem", "MaxFileDescriptorCount", max_file_descr_count);
    json.find("OperatingSystem", "SystemCpuLoad", system_cpu_load);

    if( DEBUG == true ) {
      std::cout << open_file_descr_count << std::endl;
      std::cout << max_file_descr_count << std::endl;
      std::cout << system_cpu_load << std::endl;
    }

    if(critical == 0)
      critical = max_file_descr_count;

    if(warning == 0)
      warning = critical - 300;

    if( DEBUG == true ) {
      std::cout << "warning  " << warning << std::endl;
      std::cout << "critical " << critical << std::endl;
    }

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }

  if(open_file_descr_count == warning || open_file_descr_count <= warning) {
    status = "OK";
    state = STATE_OK;
  } else
  if(open_file_descr_count >= warning || open_file_descr_count <= critical) {
    status = "WARNING";
    state = STATE_WARNING;
  } else{
    status = "CRITICAL";
    state = STATE_CRITICAL;
  }

  std::cout
    << status << " - "
    << application << " keeps " << open_file_descr_count << " files open"
    << " |"
    << " open_files=" << open_file_descr_count << ";" << warning << ";" << critical
    << std::endl;

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
  std::cout << "This plugin will return the open files count for named tomcat application" << std::endl;
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
  std::cout << " " << progname << " [-R <redis_server>] -H <hostname> -A <application> -w <warning> -c <critical>"  << std::endl;
  std::cout << std::endl;
}
