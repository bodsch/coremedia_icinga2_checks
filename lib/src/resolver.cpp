
#include "../include/resolver.h"




int Resolver::ip(const char *hostname, char *ip) {

  struct hostent *he;
  struct in_addr **addr_list;
  int i;

  he = gethostbyname(hostname);

  if(!he) {
    printf("Lookup Failed: %s\n", hstrerror(h_errno));
    return 0;
  }

  if( he == NULL) {
    // get the host info
    herror("gethostbyname");
//     switch()
    return 1;
  }

  std::cout << "Lookup: " << hostname << std::endl;
  std::cout << "h_name: " << he->h_name << std::endl;

  i = 0;
  while( he->h_aliases[i] != NULL) {
    printf("h_aliases[i]: %s\n", he->h_aliases[i]);
    i++;
  }

  printf("h_addrtype: %d - %s\n", he->h_addrtype, addrtype(he->h_addrtype));
  printf("h_length: %d\n", he->h_length);

  i = 0;
  while(he->h_addr_list[i] != NULL) {
    printf("h_addr_list[i]: %s\n", inet_ntoa( (struct in_addr) *((struct in_addr *) he->h_addr_list[i])));
    i++;
  }

  addr_list = (struct in_addr **) he->h_addr_list;

  for(i = 0; addr_list[i] != NULL; i++) {
    //Return the first one;
    strcpy(ip , inet_ntoa(*addr_list[i]) );
    return 0;
  }
}


char *Resolver::addrtype(int addrtype) {
  switch(addrtype) {
    case AF_INET:
      return (char *)"AF_INET";
    case AF_INET6:
      return (char *)"AF_INET6";
  }
  return (char *)"Unknown";
}


int Resolver::is_host(const std::string address) {

  int _addr = is_addr(address);
  int _host = is_hostname(address);

//   std::cout << "addr " << (_addr ? "true" : "false") << std::endl;
//   std::cout << "host " << (_host ? "true" : "false") << std::endl;

  if( is_addr(address) || is_hostname(address) )
    return true;

  return false;
}

/*
void Resolver::host_or_die(const std::string str) {

  if( !str.empty() || (!is_addr(str) && !is_hostname(str)))
    std::cout << "string are empty";

  if(!str || (!is_addr(str) && !is_hostname(str)))
    usage_va( printf("Invalid hostname/address - %s", str.c_str()) );
}
*/

int Resolver::is_addr(const std::string address) {
#ifdef USE_IPV6
  if (address_family == AF_INET && is_inet_addr (address))
    return true;
  else if (address_family == AF_INET6 && is_inet6_addr (address))
    return true;
#else
  if (is_inet_addr (address))
    return true;
#endif

  return false;
}


int Resolver::is_hostname(const std::string address) {

  return dns_lookup(address.c_str(), NULL, AF_INET);
}


int Resolver::is_inet_addr(const std::string address) {

  return dns_lookup(address.c_str(), NULL, AF_INET);
}


int Resolver::dns_lookup(const char *in, struct sockaddr_storage *ss, int family) {

  struct addrinfo hints;
  struct addrinfo *res;
  int retval;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = family;

  retval = getaddrinfo(in, NULL, &hints, &res);
  if(retval != 0)
    return false;

  if(ss != NULL)
    memcpy(ss, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  return true;
}

