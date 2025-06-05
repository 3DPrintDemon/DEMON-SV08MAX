#ifndef MKS_NET_INFO_H
#define MKS_NET_INFO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

bool get_interface_info(const char *interface_name, char *ip_address, char *mac_address);
bool is_interface_exist(const char *interface_name);

#endif
