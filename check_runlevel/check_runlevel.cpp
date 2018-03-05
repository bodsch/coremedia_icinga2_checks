
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

// #include <hiredis/hiredis.h>

#include <common.h>
#include <resolver.h>
#include <md5.h>
#include <redox.hpp>

#include <nlohmann/json.hpp>

const char *progname = "check_runlevel";
const char *NP_VERSION = "0.0.1";
const char *copyright = "2018";
const char *email = "bodo@boone-schulz-de";

int process_arguments (int, char **);
int validate_arguments (void);
void replaceAll(std::string& str, const std::string& from, const std::string& to);
int check( const std::string server_name, const std::string application );
void print_help (void);
void print_usage (void);

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
    std::cout << "Could not parse arguments"  << std::endl;
    return STATE_UNKNOWN;
  }

  result = check( server_name, application );

  return result;
}


int main_new(int argc, char **argv) {

  std::map<std::string,std::string> d;
  std::map<std::string,std::string>::iterator it;

  d["service"] = "foo";
  d["host"] = "osmc.local";
  d["pre"] = "result";

//  std::cout << d.size() << std::endl;

  std::string cache_key = "{";

  if( d.size() > 0 ) {

    for( it = d.begin(); it != d.end(); ++it ) {
      cache_key.append( "\"" + it->first + "\"" ); // string (key)
      cache_key.append( "=>" );
      cache_key.append( "\"" + it->second + "\"" );  // string's value

      if(std::distance(it,d.end())>1) {
        cache_key.append(", ");
      }
    }
  }

  cache_key.append("}");

  std::cout << cache_key << std::endl;
  std::cout << "md5 checksum: " << md5(cache_key) << std::endl;

  Resolver resolver;

  char ip[100];
  resolver.ip("osmc.local", ip);

  std::cout << ip  << std::endl;


//     unsigned int j;
//     redisContext *c;
//     redisReply *reply;
//
//     const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
//     int port = (argc > 2) ? atoi(argv[2]) : 6379;
//
//     struct timeval timeout = { 1, 500000 }; // 1.5 seconds
//     c = redisConnectWithTimeout(hostname, port, timeout);
//     if (c == NULL || c->err) {
//         if (c) {
//             printf("Connection error: %s\n", c->errstr);
//             redisFree(c);
//         } else {
//             printf("Connection error: can't allocate redis context\n");
//         }
//         exit(1);
//     }
//
//     /* PING server */
//     reply = redisCommand(c,"PING");
//     printf("PING: %s\n", reply->str);
//     freeReplyObject(reply);




  return 0;
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

  cache_key = "{";



  // {:host=>"osmc.local", :pre=>"result", :service=>"caefeeder-live"}
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

//   cache_key_md5 = "92b058dba63354f7a64da080556b0cd9";

//   std::cout << cache_key << std::endl;
//   std::cout << "md5 checksum: " << cache_key_md5 << std::endl;

  int state = STATE_UNKNOWN;

  Redox rdx( std::cout, log::Error);
  if(!rdx.connect("localhost", 6379)) return STATE_CRITICAL;

  // we use a named database ...
  rdx.commandSync<std::string>({"SELECT", "1"});

//   rdx.set("hello", "world!");
//   std::cout << "Hello, " << rdx.get("hello") << std::endl;

  try {
    auto x = rdx.get(cache_key_md5);

    // replace some ruby specialist things
    //
    replaceAll(x, "=>", ":");
    replaceAll(x, "nil", "\"\"");

    json j;

    try {
      j = json::parse(x);

    } catch (json::parse_error& e) {
      // output exception information
      std::cout << "message: " << e.what() << '\n'
                << "exception id: " << e.id << '\n'
                << "byte position of error: " << e.byte << std::endl;
    } catch(...) {
      std::cout << "ERROR - can\'t parse json."  << std::endl;
      state = STATE_UNKNOWN;
      return state;
    }

    int size = j.size();
/*
//     std::cout << "size: " << size << std::endl;
//    std::cout << std::setw(2) << j.dump(2) << std::endl;

    // type = j_string.type();
    if( j.is_array() )
      std::cout << "is a array" << std::endl;

    if( j.is_string() )
      std::cout << "is a string" << std::endl;

    if( j.is_object() )
      std::cout << "is a object" << std::endl;

    if( j.is_null() )
      std::cout << "is null" << std::endl;
*/
    json search;
    for( int i = 0; i <= size; ++i) {
      search = j.at(i);
      // std::cout << j.at(i) << std::endl;
      if ( search.find("Server") != search.end()) { break; }
    }

    std::cout << search.size() << std::endl;

    if( search.size() == 1 ) {

      json runlevel;
      try {
        runlevel = search["Server"]["value"];
//         std::cout << runlevel.dump(2) << std::endl;

        // get am iterator to the first element
        json::iterator it = runlevel.begin();

        runlevel = runlevel[it.key()];

        // serialize the element that the iterator points to
//         std::cout << it.key() << '\n';

        std::cout << runlevel.dump(2) << std::endl;

        auto ss = runlevel.find("Runlevel");

        std::cout << *ss << std::endl;

      //  std::transform(runlevel.begin(), runlevel.end(), runlevel.begin(), ::tolower);
      } catch(...) {

        std::cout << "ERROR - can't parse runlevel" << std::endl;
        state = STATE_CRITICAL;
        return state;
      }

//      std::cout << runlevel << std::endl;
    }
/*
    if (j.at(1).find("Memory") != j.end()) {
      std::cout << " there is an entry with key 'Memory'" << std::endl;
    }

    json::iterator it;
    for (it = j.begin(); it != j.end(); ++it) {

      std::cout << it.key() << " : " << it.value() << "\n";
//      if ( it.find("Memory") != it.end()) {
//        std::cout << " there is an entry with key 'Memory'" << std::endl;
//      }
    }
*/
/*
    for (json::iterator it = j.begin(); it != j.end(); ++it) {
      std::cout << it.key() << " : " << it.value() << "\n";
    }

// Enumerate all keys (including sub-keys -- not working)
  for (auto it=j.begin(); it!=j.end(); it++) {
    std::cout << "key: " << it.key() << " : " << it.value() << std::endl;
  }
*/
/*
      std::string d = j.dump();
      std::cout << d << std::endl;

      auto j3 = json::parse(d);
      if( j3.is_array() )
        std::cout << "is a array" << std::endl;

      if( j3.is_string() )
        std::cout << "is a string" << std::endl;

      if( j3.is_object() )
        std::cout << "is a object" << std::endl;

      if( j3.is_null() )
        std::cout << "is null" << std::endl;
*/




//       json js = json::parse(j);

/*
      for (json::iterator it = j.begin(); it != j.end(); ++it) {
        std::cout << it.key() << " : " << it.value() << "\n";
      }
*/
      //std::cout << "size " << j_string.type() << std::endl;



    state = STATE_OK;
  } catch(...) {

    std::cout << "WARNING - no data in our data store found."  << std::endl;

    state = STATE_WARNING;
  }

  rdx.disconnect();

  return(state);
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}


/* process command-line arguments */
int process_arguments (int argc, char **argv) {

  int opt = 0;
  const char* const short_opts = "hVH:a:";
  const option long_opts[] = {
    {"help"       , no_argument      , nullptr, 'h'},
    {"version"    , no_argument      , nullptr, 'V'},
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

//   std::cout << "server name: " << server_name << std::endl;
//   std::cout << "application: " << application << std::endl;

  return validate_arguments();
}

int validate_arguments(void) {

  if(server_name != NULL) {

    Resolver resolver;

    if(resolver.is_host(server_name) == false) {
      std::cout << progname << " Invalid hostname/address " << optarg << std::endl;
      return ERROR;
    }
  } else {
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

  } else {
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
  printf (" %s <integer state> [optional text]\n", progname);
}
