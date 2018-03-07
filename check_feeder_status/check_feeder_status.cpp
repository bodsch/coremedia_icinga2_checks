
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

const char *progname = "check_feeder_status";
const char *version = "1.0.0";
const char *copyright = "2018";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name, const std::string feeder );
int content_feeder( Json json );
int cae_feeder( Json json);
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *feeder = NULL;

int warn_percent = 0;
int crit_percent = 0;
int warning = 0;
int critical = 0;

enum ContentFeederState {
  STOPPED,
  STARTING,
  INITIALIZING,
  RUNNING,
  FAILED,
  UNKNOWN = 99
};


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

  result = check( server_name, feeder );

  return result;
}

/**
 *
 */
int check( const std::string server_name, const std::string feeder ) {

  int state = STATE_UNKNOWN;

  Redis r(redis_server);
  std::string cache_key = r.cache_key( server_name, feeder );

  std::string redis_data;
  if( r.get(cache_key, redis_data) == false ) {
    std::cout << "WARNING - no data in our data store found."  << std::endl;
    return STATE_WARNING;
  }

  try {

    Json json(redis_data);

    if( feeder == "content-feeder" )
      state = content_feeder( json );
    else
      state = cae_feeder( json );

  } catch(...) {

    std::cout << "WARNING - parsing of json is corrupt."  << std::endl;
    return STATE_WARNING;
  }

  return state;
}


int content_feeder( Json json ) {

  int state = STATE_UNKNOWN;

  if(warning == 0)
    warning= 2500;

  if(critical == 0)
    critical= 10000;

  std::string feeder_state = "";
  int feeder_state_numeric = UNKNOWN;
  json.find("Feeder", "State", feeder_state);
  json.find("Feeder", "StateNumeric", (int&)feeder_state_numeric );

  if( feeder_state_numeric == -1 ) {

    if( feeder_state_numeric == STOPPED ) {
      std::cout << "Feeder are in <b>stopped</b> state." << std::endl;
      state  = STATE_WARNING;
    } else
    if( feeder_state_numeric == STARTING ) {
      std::cout << "Feeder are in <b>starting</b> state." << std::endl;
      state  = STATE_UNKNOWN;
    } else
    if( feeder_state_numeric == INITIALIZING ) {
      std::cout << "Feeder are in <b>initializing</b> state." << std::endl;
      state  = STATE_UNKNOWN;
    } else
    if( feeder_state_numeric == RUNNING) {
      state  = STATE_OK;
    }
    else {
      std::cout << "Feeder are in <b>failed</b> state." << std::endl;
      state  = STATE_CRITICAL;
    }

  } else {

    std::transform(feeder_state.begin(), feeder_state.end(), feeder_state.begin(), ::tolower);

    if( feeder_state == "initializing") {
      std::cout << "Feeder are in <b>initializing</b> state." << std::endl;
      state  = STATE_WARNING;
    } else
    if( feeder_state == "running") {
      state  = STATE_OK;
    }
    else {
      std::cout << "Feeder are in <b>unknown</b> state." << std::endl;
      state  = STATE_CRITICAL;
    }
  }

  if( state == STATE_OK ) {

    int pending_documents = 0;
    int pending_events = 0;

    json.find("Feeder", "CurrentPendingDocuments", pending_documents);
    json.find("Feeder", "PendingEvents", pending_events);

    if( pending_documents != -1 ) {

      std::cout
        << "Pending Documents: " << pending_documents
        << "<br>"
        << "Pending Events: " << pending_events
        << " |"
        << " pending_documents=" << pending_documents
        << " pending_events=" << pending_events
        << std::endl;
    } else {

      std::cout
        << "Pending Events: " << pending_events
        << " |"
        << " pending_events=" << pending_events
        << std::endl;
    }
  }

  return state;
}

int cae_feeder( Json json ) {

  int state = STATE_UNKNOWN;

  if(warning == 0)
    warning= 500;

  if(critical == 0)
    critical= 800;

  bool healthy = false;
  json.find("Health", "Healthy", healthy);

  if( healthy == false ) {
    std::cout << "Feeder are not healthy!" << std::endl;
    return STATE_CRITICAL;
  }

  int max_entries = 0;
  int current_entries = 0;
  int heartbeat = 0;

  json.find("ProactiveEngine", "KeysCount", max_entries); // Number of (active) keys
  json.find("ProactiveEngine", "ValuesCount", current_entries); // Number of (valid) values. It is less or equal to 'keysCount'
  json.find("ProactiveEngine", "HeartBeat", heartbeat); // 1 minute == 60000 ms

  if( max_entries == 0 && current_entries == 0 ) {

    std::cout
      << max_entries << "Elements for Feeder available. This feeder is maybe restarting?"
      << std::endl;

    return STATE_UNKNOWN;
  }

  if( heartbeat >= 60000 ) {

    std::cout
      << "Feeder Heartbeat is more then 1 minute old."
      << "<br>"
      << "Heartbeat " << heartbeat << " ms"
      << std::endl;

    return STATE_CRITICAL;
  }

  int diff_entries = max_entries - current_entries;

  if( diff_entries > critical ) {
    state = STATE_CRITICAL;
  } else
  if( diff_entries > warning || diff_entries == warning ) {
    state = STATE_WARNING;
  } else {
    state = STATE_OK;
  }

  if( max_entries == current_entries ) {

    std::cout
      << "all " << max_entries << " Elements feeded"
      << "<br>"
      << "Heartbeat " << heartbeat << " ms"
      << " |"
      << " max=" << max_entries
      << " current=" << current_entries
      << " diff=" << diff_entries
      << " heartbeat=" << heartbeat
      << std::endl;
  } else {

    std::cout
      << current_entries << " Elements of " << max_entries << " feeded."
      << "<br>"
      << diff_entries << " elements left."
      << "<br>"
      << "Heartbeat " << heartbeat << " ms"
      << " |"
      << " max=" << max_entries
      << " current=" << current_entries
      << " diff=" << diff_entries
      << " heartbeat=" << heartbeat
      << std::endl;
  }

  return state;
}

/*
 * process command-line arguments
 */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:F:w:c:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"feeder"     , required_argument, nullptr, 'F'},
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
      case 'F':
        feeder = optarg;
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

  if(feeder != NULL)  {

    std::vector<std::string> app = {
      "content-feeder",
      "caefeeder-live",
      "caefeeder-previews"
    };

    if( in_array( feeder, app )) {
      return OK;
    } else {
      std::cerr << "'" << feeder << "' is no valid content server!" << std::endl;
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
  std::cout << "  - content-feeder, " << std::endl;
  std::cout << "  - caefeeder-live and " << std::endl;
  std::cout << "  - caefeeder-preview" << std::endl;
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
  std::cout << " -F, --feeder" << std::endl;
  std::cout << "    the feeder you want to test." << std::endl;

}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " [-R <redis_server>] -H <hostname> -F <feeder>"  << std::endl;
  std::cout << std::endl;
}
