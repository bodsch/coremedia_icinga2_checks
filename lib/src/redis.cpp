
#include "../include/redis.h"

std::string Redis::cache_key(const std::string hostname, const std::string service) {

  /**
   * calculate the cache key for the data in our redis */
  std::map<std::string,std::string> d;
  std::map<std::string,std::string>::iterator it;

  // cacheKey = Storage::RedisClient.cacheKey( { :host => host, :pre => 'result', :service => service } )

  d["service"] = service;
  d["host"] = hostname;
  d["pre"] = "result";

  std::string key;

  // {:host=>"osmc.local", :pre=>"result", :service=>"caefeeder-live"}
  key = "{";
  if( d.size() > 0 ) {
    for( it = d.begin(); it != d.end(); ++it ) {
      key.append( ":" + it->first ); // string (key)
      key.append( "=>" );
      key.append( "\"" + it->second + "\"" );  // string's value

      if(std::distance(it,d.end())>1) {
        key.append(", ");
      }
    }
  }
  key.append("}");
  return md5(key);
}



bool Redis::get(const std::string key, std::string &result) {

  redox::Redox rdx( std::cerr, redox::log::Off);

  if(!rdx.connect(_server, _port)) {
    std::cout << "ERROR - Could not connect to Redis: Connection refused." << std::endl;
    return false;
  }

  // we use a named database ...
  rdx.commandSync<std::string>({"SELECT", "1"});

  try {
    result = rdx.get(key);
  } catch(...) {
    std::cout << "WARNING - no data in our data store found."  << std::endl;
    return false;
  }

  rdx.disconnect();

  return true;
}

