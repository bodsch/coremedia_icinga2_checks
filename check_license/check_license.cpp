
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <cctype>
#include <vector>
#include <limits>
#include <ctime>
#include <algorithm>
#include <map>
#include <getopt.h>

#include <common.h>
#include <resolver.h>
#include <md5.h>

#include <redis.h>
#include <json.h>

const char *progname = "check_license";
const char *version = "1.0.0";
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

int warning = 0;
int critical = 0;

struct MTime {
  int years;
  int months;
  int weeks;
  int days;
  int hours;
  int minutes;
  int seconds;
};

MTime time_difference( const std::time_t start_time, const std::time_t end_time );

/**
 *
 */
int main(int argc, char **argv) {

  int result = STATE_UNKNOWN;

  if( process_arguments(argc, argv) == ERROR ) {
    std::cout << std::endl;
    print_usage();
    return STATE_UNKNOWN;
  }

  result = check( server_name, content_server );

  return result;
}

/**
 *
 */
int check( const std::string server_name, const std::string content_server ) {

  int state = STATE_UNKNOWN;

  if(warning == 0)
    warning= 50;

  if(critical == 0)
    critical= 20;

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, content_server );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    std::cout << "WARNING - no data in our data store found."  << std::endl;
    return STATE_WARNING;
  }

  try {

    Json json(redis_data);

    long valid_until_hard = 0;
    long valid_until_soft = 0;
    json.find("Server", "LicenseValidUntilHard", valid_until_hard);
    json.find("Server", "LicenseValidUntilSoft", valid_until_soft);

    /**
     * calculate time
     * valid hard is the last chance
     * valid soft are the entry into a soft warning phase
     *
     * MTime struct holds all diffs between now and valid_until_hard
     */
    std::time_t valid_hard = valid_until_hard / 1000;
    std::time_t now = time(0);   // get time now
    MTime time_diff = time_difference( now, valid_until_hard  / 1000 );

    int valid_until_days = time_diff.days;

    struct tm * timeinfo;
    timeinfo = localtime(&valid_hard);

    char license_date[11];
    strftime(license_date, sizeof(license_date), "%d.%m.%Y", timeinfo);


    if( valid_until_days >= warning || valid_until_days == warning ) {
      state = STATE_OK;
    } else
    if( valid_until_days >= critical && valid_until_days <= warning ) {
      state = STATE_WARNING;
    } else {
      state = STATE_CRITICAL;
    }

    std::cout
      << "<b>" << valid_until_days << " days left</b>"
      << "<br>"
      << "CoreMedia License is valid until " << license_date
      << " |"
      << " valid=" << valid_until_days
      << " warning=" << warning
      << " critical=" << critical
      << std::endl;

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
  const char* const short_opts = "hVR:H:C:w:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"contentserver", required_argument, nullptr, 'C'},
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
      case 'C':
        content_server = optarg;
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

  if( warning < critical ) {
    std::cout << "Warning should be more than critical" << std::endl;
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
  std::cout << " " << progname << " [-R <redis_server>] -H <hostname> -C <content_server>"  << std::endl;
  std::cout << std::endl;
}

MTime time_difference( const std::time_t start_time, const std::time_t end_time ) {

  const unsigned int YEAR   = 1*60*60*24*7*52;
  const unsigned int MONTH  = YEAR/12;
  const unsigned int WEEK   = 1*60*60*24*7;
  const unsigned int DAY    = 1*60*60*24;
  const unsigned int HOUR   = 1*60*60;
  const unsigned int MINUTE = 1*60;

  long difference = difftime( end_time, start_time );

  MTime t;

  t.years   = difference / YEAR;
  t.months  = difference / MONTH;
  t.weeks   = difference / WEEK;
  t.days    = difference / DAY;
  t.hours   = difference / HOUR;
  t.minutes = difference / MINUTE;
  t.seconds = difference;

  return t;
}


