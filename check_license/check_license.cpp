
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

const char *progname = "check_license";
const char *version = "1.1.1";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name, const std::string content_server );
int license( long lic, std::string& license_date );
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *content_server = NULL;

bool check_valid_soft = false;
bool check_valid_hard = false;

int warning_soft  = 80;
int warning_hard  = 40;
int critical_soft = 20;
int critical_hard = 10;

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

  std::string status = "";

  try {

    Json json(redis_data);

    long valid_until_hard = 0;
    long valid_until_soft = 0;
    json.find("Server", "LicenseValidUntilHard", valid_until_hard);
    json.find("Server", "LicenseValidUntilSoft", valid_until_soft);

    std::string license_date_soft = "";
    std::string license_date_hard = "";

    int days_left_soft = license( valid_until_hard, license_date_soft );
    int days_left_hard = license( valid_until_soft, license_date_hard );

    std::ostringstream ss;

    if( check_valid_soft ) {

      if( days_left_soft <= critical_soft ) {
        status = "CRITICAL"; state = STATE_CRITICAL;
      } else
      if( days_left_soft > critical_soft && days_left_soft < warning_soft ) {
        status = "WARNING";  state = STATE_WARNING;
      } else {
        status = "OK";       state = STATE_OK;
      }

      ss
        << "soft: CoreMedia license is valid until " << license_date_soft
        << " - <b>" << days_left_soft << " days left</b> "
        << "(" << status << ")";
    }

    if( check_valid_hard ) {

      if( days_left_hard <= critical_hard ) {
        status = "CRITICAL"; state = STATE_CRITICAL;
      } else
      if( days_left_hard > critical_hard && days_left_hard < warning_hard ) {
        status = "WARNING";  state = STATE_WARNING;
      } else {
        status = "OK";       state = STATE_OK;
      }

      if( ss.str().size() != 0 ) {
        ss << "<br>";
        std::cout << ss.str() << std::endl;
        ss.str(std::string());
      }

      ss
        << "hard: CoreMedia license is valid until " << license_date_hard
        << " - <b>" << days_left_hard << " days left</b> "
        << "(" << status << ")";
    }

    ss << " |";

    if( check_valid_soft ) {
      ss << " valid_soft=" << days_left_soft;
    }
    if( check_valid_hard ) {
      ss << " valid_hard=" << days_left_hard;
    }

    std::cout << ss.str() << std::endl;

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }

  return state;
}

/**
 * calculate time
 * valid hard is the last chance
 * valid soft are the entry into a soft warning phase
 *
 * TimeDifference struct holds all diffs between now and valid_until
 */
int license( long lic, std::string& date ) {

  std::time_t valid = lic / 1000;
  std::time_t now = time(0);   // get time now
  TimeDifference time_diff = time_difference( now, lic  / 1000 );

  int days_left = time_diff.days;

  struct tm * timeinfo;
  timeinfo = localtime(&valid);

  char license_date[11];
  strftime(license_date, sizeof(license_date), "%d.%m.%Y", timeinfo);

  date = license_date;

  return days_left;
}

/*
 * process command-line arguments
 */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:C:tdw:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"contentserver", required_argument, nullptr, 'C'},
    {"soft"       , no_argument      , nullptr, 't' },
    {"hard"       , no_argument      , nullptr, 'd' },
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
      case 't':
        check_valid_soft = true;
        break;
      case 'd':
        check_valid_hard = true;
        break;
      case 'w':                  /* warning size threshold */
        if(is_intnonneg(optarg)) {
          warning_soft = warning_hard = atoi(optarg);
          break;
        } else
        if( strstr(optarg, ",") &&
            sscanf(optarg, "%d,%d", &warning_soft, &warning_hard) == 2) {
          warning_soft = warning_hard = atoi(optarg);
          break;
        } else {
          std::cout << "Warning threshold must be integer!" << std::endl;
          print_usage();
        }
      case 'c':                  /* critical size threshold */
        if(is_intnonneg(optarg)) {
          critical_soft = critical_hard = atoi(optarg);
          break;
        } else
        if( strstr(optarg, ",") &&
            sscanf(optarg, "%d,%d", &critical_soft, &critical_hard) == 2) {
          critical_soft = critical_hard = atoi(optarg);
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

  int result = OK;
  Resolver resolver;

  if(redis_server == NULL)
    redis_server = std::getenv("REDIS_HOST");

  if(redis_server != NULL) {

    if(resolver.is_host(redis_server) == false) {
      std::cout << progname << " Invalid redis host or address " << optarg << std::endl;
      result = ERROR;
    }
  } else {
    std::cerr << "missing 'redis' argument." << std::endl;
    result = ERROR;
  }

  if(server_name != NULL) {

    if(resolver.is_host(server_name) == false) {
      std::cout << progname << " Invalid hostname or address " << optarg << std::endl;
      result = ERROR;
    }
  } else {
    std::cerr << "missing 'hostname' argument." << std::endl;
    result = ERROR;
  }

  if(content_server != NULL)  {

    std::vector<std::string> app = {
      "content-management-server",
      "master-live-server",
      "replication-live-server"
    };

    if( in_array( content_server, app )) {
      result = OK;
    } else {
      std::cerr << "'" << content_server << "' is no valid content server!" << std::endl;
      result = ERROR;
    }
  } else {
    std::cerr << "missing 'contentserver' argument." << std::endl;
    result = ERROR;
  }

  if( check_valid_soft == false && check_valid_hard == false ) {
    std::cerr << "please choose --soft and/or --hard for license check." << std::endl;
    result = ERROR;
  }

  if( warning_soft < critical_soft || warning_hard < critical_hard ) {
    std::cout << "Warning should be more than critical" << std::endl;
  }

  return result;
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

  std::cout << " -t, --soft" << std::endl;
  std::cout << "    license valid until soft." << std::endl;
  std::cout << " -d, --hard" << std::endl;
  std::cout << "    license valid until hard." << std::endl;

  std::cout << " -w, --warning=WSOFT,WHARD" << std::endl;
  std::cout << "    exit with WARNING status if license average exceeds WSOFT or WHARD." << std::endl;
  std::cout << " -c, --critical=WSOFT,CHARD" << std::endl;
  std::cout << "    exit with CRITICAL status if license average exceeds CSOFT or CHARD." << std::endl;
}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout
    << " " << progname
    << " [-R <redis_server>]"
    << " -H <hostname>"
    << " -C <content_server>"
    << " --hard"
    << " [--soft]"
    << " [-w <WSOFT,WHARD>]"
    << " [-c <CSOFT,CHARD>]"
    << std::endl;
  std::cout << std::endl;
}

