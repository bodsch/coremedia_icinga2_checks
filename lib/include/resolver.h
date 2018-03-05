
#ifndef RESOLVER_H
#define RESOLVER_H

#include <cstdlib>
#include <iostream>
#include <cstring>

#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //for exit(0);
#include <sys/socket.h>
#include <errno.h> //For errno - the error number
#include <netdb.h> //hostent

#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
/*
#define resolve_host_or_addr(addr, family) dns_lookup(addr, NULL, family)
#define is_inet_addr(addr) resolve_host_or_addr(addr, AF_INET)

#ifdef USE_IPV6
#  define is_inet6_addr(addr) resolve_host_or_addr(addr, AF_INET6)
#  define is_hostname(addr) resolve_host_or_addr(addr, address_family)
#else
#  define is_hostname(addr) resolve_host_or_addr(addr, AF_INET)
#endif
*/
class Resolver {
  public:
                Resolver() {}

    int         ip(const char *hostname, char *ip);

    int         is_host(const std::string address);
    void        host_or_die(const std::string str);
    int         is_addr(const std::string address);
    int         dns_lookup(const char *in, struct sockaddr_storage *ss, int family);
    int         is_hostname(const std::string address);
    int         is_inet_addr(const std::string address);

  private:
    char        *addrtype(int addrtype);
};

#endif // RESOLVER_H


