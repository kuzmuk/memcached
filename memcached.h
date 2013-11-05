/*
   2013 (c) Alex Kuzmuk
   github@kuzmuk.com
*/

#ifndef __MEMCACHED_H
#define __MEMCACHED_H

#include "ini.h"
#include "config.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>


int initMemcache();
void closeMemcache();
int memcache_set(char * key, unsigned int keylen, char * value, unsigned int vallen, unsigned int expire);
int memcache_get(char * key, unsigned int keylen, char * value);

#endif
