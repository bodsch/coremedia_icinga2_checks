
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
#include <json.h>

#include <redox.hpp>

const char *progname = "check_runlevel";
const char *NP_VERSION = "1.0.0";
const char *copyright = "2018";
const char *email = "bodo@boone-schulz-de";

int process_arguments (int, char **);
int validate_arguments (void);
int check( const std::string server_name, const std::string application );
bool in_array(const std::string &value, const std::vector<std::string> &array);
void print_help (void);
void print_usage (void);

char *redis_server = NULL;
char *server_name = NULL;
char *application = NULL;
bool verbose = false;

using namespace redox;
using nlohmann::json;

/*
  opts.on('-h', '--host NAME'       , 'Host with running Application')                   { |v| options[:host]  = v }
  opts.on('-a', '--application APP' , 'Name of the running Application')                 { |v| options[:application]  = v }
*/

int main(int argc, char **argv) {

  int result = STATE_UNKNOWN;

  if( process_arguments(argc, argv) == ERROR ) {
//     std::cout << "Could not parse arguments"  << std::endl;
    std::cout << std::endl;
    print_usage();
    return STATE_UNKNOWN;
  }

  result = check( server_name, application );

  return result;
}


int check( const std::string server_name, const std::string application ) {

  /**
   * calculate the cache key for the data in our redis */
  std::map<std::string,std::string> d;
  std::map<std::string,std::string>::iterator it;

  // cacheKey = Storage::RedisClient.cacheKey( { :host => host, :pre => 'result', :service => service } )

  d["service"] = application;
  d["host"] = server_name;
  d["pre"] = "result";

  std::string cache_key;
  std::string cache_key_md5;

  // {:host=>"osmc.local", :pre=>"result", :service=>"caefeeder-live"}
  cache_key = "{";
  if( d.size() > 0 ) {
    for( it = d.begin(); it != d.end(); ++it ) {
      cache_key.append( ":" + it->first ); // string (key)
      cache_key.append( "=>" );
      cache_key.append( "\"" + it->second + "\"" );  // string's value

      if(std::distance(it,d.end())>1) {
        cache_key.append(", ");
      }
    }
  }
  cache_key.append("}");
  cache_key_md5 = md5(cache_key);

  int state = STATE_UNKNOWN;

  Redox rdx( std::cerr, log::Off);

  if(!rdx.connect(redis_server, 6379)) {

    std::cout << "ERROR - Could not connect to Redis: Connection refused." << std::endl;
    return STATE_CRITICAL;
  }

  // we use a named database ...
  rdx.commandSync<std::string>({"SELECT", "1"});

  try {

    auto x = rdx.get(cache_key_md5);

    Json json(x);

    std::string runlevel = "";
    int runlevel_numeric = 0;
    json.find("Server", "RunLevel", runlevel);
    json.find("Server", "RunLevelNumeric", runlevel_numeric);

    std::string status = "";

    if( runlevel_numeric == -1 ) {

      if( runlevel_numeric == 0) { runlevel = "stopped"; state  = STATE_WARNING; }
      else
      if( runlevel_numeric == 1) { runlevel = "starting"; state  = STATE_UNKNOWN; }
      else
      if( runlevel_numeric == 2) { runlevel = "initializing"; state  = STATE_UNKNOWN; }
      else
      if( runlevel_numeric == 3) { runlevel = "running"; state  = STATE_OK; }
      else { runlevel = "failed"; state  = STATE_CRITICAL; }

    } else {

      std::transform(runlevel.begin(), runlevel.end(), runlevel.begin(), ::tolower);

      if(runlevel == "offline") { status = "CRITICAL"; state  = STATE_CRITICAL; }
      else
      if( runlevel == "online") { status = "OK"; state  = STATE_OK; }
      else
      if( runlevel == "administration") { status = "WARNING"; state  = STATE_WARNING; }
      else { status = "CRITICAL"; state  = STATE_CRITICAL; }
    }

    std::cout << "Runlevel in <b>" << runlevel << "</b> Mode" << std::endl;

  } catch(...) {

    std::cout << "WARNING - no data in our data store found."  << std::endl;

    state = STATE_WARNING;
  }

  rdx.disconnect();

  return(state);
}


/* process command-line arguments */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVR:H:a:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
    {"redis"      , required_argument, nullptr, 'R'},
    {"hostname"   , required_argument, nullptr, 'H'},
    {"application", required_argument, nullptr, 'a'},
    {nullptr      , 0, nullptr, 0}
  };

  if(argc < 2)
    return ERROR;

  int long_index =0;
  while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1) {

    switch (opt) {
      case 'h':                  /* help */
        print_usage();
        print_help();
        exit(STATE_UNKNOWN);
      case 'V':                  /* version */
        print_revision (progname, NP_VERSION);
        exit(STATE_UNKNOWN);
      case 'R':                  /* redis */
        redis_server = optarg;
        break;
      case 'H':                  /* host */
        server_name = optarg;
        break;
      case 'a':                  /* application */
        application = optarg;
        break;
      default:
        print_usage();
        break;
    }
  }

  return validate_arguments();
}

int validate_arguments(void) {

  if(redis_server != NULL) {

    Resolver resolver;

    if(resolver.is_host(redis_server) == false) {
      std::cout << progname << " Invalid redis host or address " << optarg << std::endl;
      return ERROR;
    }
  } else {
    std::cerr << "missing 'redis' argument." << std::endl;
    return ERROR;
  }

  if(server_name != NULL) {

    Resolver resolver;

    if(resolver.is_host(server_name) == false) {
      std::cout << progname << " Invalid hostname or address " << optarg << std::endl;
      return ERROR;
    }
  } else {
    std::cerr << "missing 'hostname' argument." << std::endl;
    return ERROR;
  }

/*
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"content-management-server"}
  redis cache_key: 4583589d6518613d423a47fb5909b1fe
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"master-live-server"}
  redis cache_key: f6543a8a5390117b6867886c0f2390c9
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"workflow-server"}
  redis cache_key: 80a8df93a48388d1b4045a5a7389d523
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"content-feeder"}
  redis cache_key: 9a0f1622748fa0dea01e5a63a779d5a6
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"user-changes"}
  redis cache_key: 3c3ff8787f47660e414832122e387da8
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"elastic-worker"}
  redis cache_key: de62ce017cd797f3057f520f24faa5ac
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"caefeeder-preview"}
  redis cache_key: d3495b309aa92e41c5c9dd8f98ee3f97
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"caefeeder-live"}
  redis cache_key: a5783769d8d36c6375e6128829abedc4
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"cae-preview"}
  redis cache_key: d143579c9e5de311f911c45c1f2e4502
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"studio"}
  redis cache_key: ae43845307ff9148c7944629ec9b3c45
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"sitemanager"}
  redis cache_key: 812b84d2dfee88a5e954b848444f09f5
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"replication-live-server"}
  redis cache_key: 19745983dd637044b2e3d9790ff041b0
  plain cache_key: {:host=>"osmc.local", :pre=>"result", :service=>"cae-live"}
*/


  /**
   * TODO
   *  valid application are:
   *   - content-management-server
   *   - master-live-server
   *   - replication-live-server
   */

  if(application != NULL)  {

    std::vector<std::string> content_server = { "content-management-server", "master-live-server", "replication-live-server" };

    if( in_array( application, content_server )) {
      return OK;
    } else {
      std::cerr << "'" << application << "' is no valid content server!" << std::endl;
      return ERROR;
    }

  } else {
    std::cerr << "missing 'application' argument." << std::endl;
    return ERROR;
  }

  return OK;
}



void print_help (void) {

  printf ("%s v%s (%s %s)\n", progname, NP_VERSION, PACKAGE, VERSION);

//   print_revision (progname, NP_VERSION);
  printf (COPYRIGHT, copyright, email);

  printf ("%s\n", ("This plugin will simply return the state corresponding to the numeric value"));
  printf ("%s\n", ("of the <state> argument with optional text"));
  printf ("\n\n");
  print_usage ();
  printf (UT_HELP_VRSN);
}

void print_usage (void) {
  printf ("%s\n", ("Usage:"));
  printf (" %s -R <redis_server> -H <host_name> -a <application name>\n", progname);
}


bool in_array(const std::string &value, const std::vector<std::string> &array) {
  return std::find(array.begin(), array.end(), value) != array.end();
}
