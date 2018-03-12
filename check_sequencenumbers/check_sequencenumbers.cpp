
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

const char *progname = "check_sequencenumbers";
const char *version = "1.0.3";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const nlohmann::json rls, nlohmann::json mls );
bool extract_data( Redis r, std::string app, nlohmann::json &result );
bool detect_mls( Redis r, const std::string rls, std::string &result);

void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *rls = NULL;
char *mls = NULL;

int warn_percent = 0;
int crit_percent = 0;
int warning = 150;
int critical = 200;

/**
 *
 */
int main(int argc, char **argv) {

  if( process_arguments(argc, argv) == ERROR ) {
    std::cout << std::endl;
    print_usage();
    return STATE_UNKNOWN;
  }

  Redis r(redis_server);

  if(mls == NULL) {

    std::string _mls;
    if( detect_mls(r, rls, _mls) == false ) {
      std::cout << "can't auto detect the master-liver-server hostname" << std::endl;
      return STATE_CRITICAL;
    } else {
      mls = new char[_mls.length() + 1];
      strcpy(mls, _mls.c_str());
    }
  }

  nlohmann::json rls_data = "";
  nlohmann::json mls_data = "";

  bool rls_found = extract_data( r, "replication-live-server", rls_data );
  bool mls_found = extract_data( r, "master-live-server", mls_data );

  if( rls_found == false ) {
    std::cout << "no data for the replication-liver-server found" << std::endl;
    return STATE_CRITICAL;
  }

  if( mls_found == false ) {
    std::cout << "no data for the master-liver-server found" << std::endl;
    return STATE_CRITICAL;
  }

  return check( rls_data, mls_data );
}

/**
 *
 */
int check( const nlohmann::json rls, nlohmann::json mls ) {

  std::string status = "";
  int state = STATE_UNKNOWN;

  int mls_sequence_number = 0;
  std::string mls_runlevel = "";
  int mls_runlevel_numeric = -1;

  int rls_sequence_number = 0;
  std::string rls_controller_state = "";

  try {
    if( rls != "" ) {

      auto it_find_sequence_number  = rls.find("LatestCompletedSequenceNumber");
      auto it_find_controller_state = rls.find("ControllerState");

      if(it_find_sequence_number != rls.end())
        rls_sequence_number = rls["LatestCompletedSequenceNumber"];

      if(it_find_controller_state != rls.end())
        rls_controller_state = rls["ControllerState"];
    }

  } catch(...) {
    std::cout << "WARNING - error in parsing the rls data" << std::endl;
    return STATE_WARNING;
  }

  try {
    if( mls != "" ) {

      auto it_find_sequence_number  = mls.find("RepositorySequenceNumber");
      auto it_find_runlevel_numeric = mls.find("RunLevelNumeric");
      auto it_find_runlevel         = mls.find("RunLevel");

      if(it_find_sequence_number != mls.end())
        mls_sequence_number = mls["RepositorySequenceNumber"];

      if(it_find_runlevel_numeric != mls.end())
        mls_runlevel_numeric = mls["RunLevelNumeric"];

      if(it_find_runlevel != mls.end())
        mls_runlevel = mls["RunLevel"];
    }

  } catch(...) {
    std::cout << "WARNING - error in parsing the mls data" << std::endl;
    return STATE_WARNING;
  }

  int mls_state = service_state(mls_runlevel,mls_runlevel_numeric);
  int rls_state = service_state(rls_controller_state, UNDEFINED);

  if( rls_state != STATE_OK ) {
    return rls_state;
  }

  if( mls_state != STATE_OK ) {
    return mls_state;
  }

  int diff = mls_sequence_number - rls_sequence_number;

  if( diff == warning || diff <= warning ) {

    std::cout
      << "OK - "
      << "RLS and MLS in sync"
      << "<br>"
      << "MLS Sequence Number: " << mls_sequence_number
      << "<br>"
      << "RLS Sequence Number: " << rls_sequence_number
      << " |"
      << " diff=" << diff
      << " mls_seq_nr=" << mls_sequence_number
      << " rls_seq_nr=" << rls_sequence_number
      << std::endl;

    return STATE_OK;
  }

  if( diff >= warning && diff <= critical ) {
    status = "WARNING";
    state = STATE_WARNING;
  } else {
    status = "CRITICAL";
    state = STATE_CRITICAL;
  }

  std::cout
    << status << " - "
    << "RLS are " << diff << " <b>behind</b> the MLS"
    << "<br>"
    << "MLS Sequence Number: " << mls_sequence_number
    << "<br>"
    << "RLS Sequence Number: " << rls_sequence_number
    << " |"
    << " diff=" << diff
    << ";" << warning
    << ";" << critical
    << " mls_seq_nr=" << mls_sequence_number
    << " rls_seq_nr=" << rls_sequence_number
    << std::endl;

  return state;
}



bool detect_mls( Redis r, const std::string rls, std::string &result) {

  std::string cache_key = r.cache_key( rls, "replication-live-server" );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    return false;
  }

  if( redis_data.empty() || redis_data.size() == 2 ) {
    return false;
  }

  try {

    Json json(redis_data);

    nlohmann::json master_live_server = "";

    json.find("Replicator", "MasterLiveServer", master_live_server);

    if( master_live_server != "" ){
//       std::cout << master_live_server.dump(2) <<  std::endl;
      result = master_live_server["host"];
    } else {
//       std::cout << "can't auto detect the master-liver-server hostname" << std::endl;
      return false;
    }

  } catch(...) {
//     std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return false;
  }

  return true;
}


bool extract_data( Redis r, std::string app, nlohmann::json &result ) {

  std::string find = "Replicator";

  if( app == "master-live-server" )
    find = "Server";

  std::string cache_key = r.cache_key( rls, app );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    return false;
  }

  if( redis_data.empty() || redis_data.size() == 2 ) {
    std::cout << "no data found" << std::endl;
    return false;
  }

  try {

    Json json(redis_data);
    json.find( find , result );

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return false;
  }

  return true;
}


/*
 * process command-line arguments
 */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:r:m:w:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"rls"        , required_argument, nullptr, 'r'},
    {"mls"        , required_argument, nullptr, 'm'},
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
      case 'r':
        rls = optarg;
        break;
      case 'm':
        mls = optarg;
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

  if(rls != NULL) {

    if(resolver.is_host(rls) == false) {
      std::cout << progname << " Invalid hostname or address for the RLS " << optarg << std::endl;
      return ERROR;
    }
  } else {
    std::cerr << "missing 'rls' argument." << std::endl;
    return ERROR;
  }

  if(mls != NULL)  {

    if(resolver.is_host(mls) == false) {
      std::cout << progname << " Invalid hostname or address for the MLS " << optarg << std::endl;
      return ERROR;
    }
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
  std::cout << "This plugin will check the differece of sequencenumbers between an replication-liver-ser and his master-live-server" << std::endl;
  std::cout << "Since CoreMedia 170x, the RLS announces its MLS." << std::endl;
  std::cout << "If the master-live-server is not specified, it will be attempted to determine it automatically." << std::endl;
  std::cout << "But this only works from version 170x!" << std::endl;
  print_usage();
  std::cout << "Options:" << std::endl;
  std::cout << " -h, --help" << std::endl;
  std::cout << "    Print detailed help screen" << std::endl;
  std::cout << " -V, --version" << std::endl;
  std::cout << "    Print version information" << std::endl;
  std::cout << " -R, --redis" << std::endl;
  std::cout << "    (optional) the redis service who stored the measurements data." << std::endl;
  std::cout << " -r, --rls" << std::endl;
  std::cout << "    the hostname of the replication-live-server." << std::endl;
  std::cout << " -m, --mls" << std::endl;
  std::cout << "    the hostname of the master-live-server." << std::endl;

}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " [-R <redis_server>] -r <replication-live-server> -m <master-live-server>"  << std::endl;
  std::cout << std::endl;
}
