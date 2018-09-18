
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

const char *progname = "check_nodeexporter_disk";
const char *version = "1.0.3";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name );

void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *partition_name = NULL;

int warn_percent = 85;
int crit_percent = 90;
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

  char buf[15];

  try {

    Json json(redis_data);

    if( DEBUG == true ) {
      std::cout << redis_data << std::endl;
    }

    nlohmann::json filesystem = "";
    json.find("filesystem", filesystem);
/*
    if( DEBUG == true ) {
      std::cout << "filesystem : ";
      if( filesystem.is_null() )    { std::cout << "is null" << std::endl; }
      if( filesystem.is_boolean() ) { std::cout << "is boolean" << std::endl; }
      if( filesystem.is_number() )  { std::cout << "is number" << std::endl; }
      if( filesystem.is_object() )  { std::cout << "is object" << std::endl; }
      if( filesystem.is_array() )   { std::cout << "is array" << std::endl; }
      if( filesystem.is_string() )  { std::cout << "is string" << std::endl; }
    }
*/
    if( !filesystem.is_object() ) {
      std::cout
        << "<b>no filesystem measurement points available.</b>"
        << std::endl;
      return STATE_WARNING;
    }

    if( DEBUG == true ) {
      std::cout << filesystem << std::endl;
    }

    if( filesystem.size() >= 1 ) {

      std::string mountpoint = "";
      long avail = 0;
      long size = 0;
      long used = 0;
      int used_percent = 0;
      std::string size_human_readable = "";
      std::string used_human_readable = "";
      std::string avail_human_readable = "";

      std::string key = "";
      nlohmann::json value = "";

      for( auto it = filesystem.begin(); it != filesystem.end(); ++it ) {

        key   = it.key();
        value = it.value();

        if( DEBUG == true ) {
/*
          std::cout << "value : ";
          if( value.is_null() )    { std::cout << "is null" << std::endl; }
          if( value.is_boolean() ) { std::cout << "is boolean" << std::endl; }
          if( value.is_number() )  { std::cout << "is number" << std::endl; }
          if( value.is_object() )  { std::cout << "is object" << std::endl; }
          if( value.is_array() )   { std::cout << "is array" << std::endl; }
          if( value.is_string() )  { std::cout << "is string" << std::endl; }
*/
          std::cout << value << std::endl;
        }

        if( value.is_object() ) {
          mountpoint = filesystem[key]["mountpoint"];
          if(mountpoint == partition_name) {
            break;
          } else {
            key = "";
          }
        }
      }

      if( DEBUG == true ) {
        std::cout << key << std::endl;
      }

      if( key.empty() ) {
        std::cout
          << "partition '" << partition_name << "' not found."
          << std::endl;

        return STATE_UNKNOWN;
      }

      auto it_find_size  = value.find("size");
      auto it_find_avail = value.find("avail");
      auto it_find_used = value.find("used");
      auto it_find_used_percent = value.find("used_percent");

      if(it_find_size != value.end())
        size = value["size"];

      if(it_find_avail != value.end())
        avail = value["avail"];

      if(it_find_used != value.end())
        used = value["used"];

      if(it_find_used_percent != value.end())
        used_percent = value["used_percent"];

      if( size > 0 ) {

        if( used == 0 && used_percent == 0) {
          used  = size - avail;
          used_percent  = 100 * used / size;

          used_percent  = roundf(used_percent *100) / 100;
          used_percent  = ((int)(used_percent * 100 + .5) / 100.0);
        }

        size_human_readable  = human_readable(size,buf);
        used_human_readable  = human_readable(used,buf);
        avail_human_readable = human_readable(avail,buf);

        if( used_percent == warn_percent || used_percent <= warn_percent ) {
          status    = "OK";
          state = STATE_OK;
        } else
        if( used_percent >= warn_percent && used_percent <= crit_percent ) {
          status    = "WARNING";
          state = STATE_WARNING;
        } else {
          status    = "CRITICAL";
          state = STATE_CRITICAL;
        }

        std::cout
          << status << " - "
          << "partition '" << partition_name << "' - "
          << "size: " << size_human_readable << ", "
          << "used: " << used_human_readable << ", "
          << "used percent: " << used_percent << "% |"
          << " size=" << size
          << " used=" <<  used
          << " used_percent=" << used_percent
          << std::endl;
      }
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
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:DP:w:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"partition"  , required_argument, nullptr, 'P'},
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
      case 'P':
        partition_name = optarg;
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

  if(partition_name == NULL) {
    std::cerr << "missing 'partition' argument." << std::endl;
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
  std::cout << " -P, --partition" << std::endl;
  std::cout << "    the partition to be checked." << std::endl;
}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " [-R <redis_server>] -H <hostname> -P <partition name>"  << std::endl;
  std::cout << std::endl;
}
