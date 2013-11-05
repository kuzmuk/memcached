/*

   2013 (c) Alex Kuzmuk
   github@kuzmuk.com
   
*/

#include "memcached.h"

int memSock;

void closeMemcache() {
  close(memSock);
}

int initMemcache() {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.memcachedPort);
    addr.sin_addr.s_addr = inet_addr(config.memcachedServer); 

    if ((memSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
          wlog("Could not open socket for memcached server");
    }
    if (connect(memSock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
          wlog("Failed to connect with server");
    }

  return 0;
}
int memcache_set(char * key, unsigned int keylen, char * value, unsigned int vallen, unsigned int expire) {
    unsigned char * message;
    unsigned char * resp;
    unsigned int headerLength, bodyLenght, packetLength;
    int received, res;
    res = 0;
    headerLength = 24;
    bodyLenght = vallen + keylen + 8;
    packetLength = headerLength + bodyLenght; 
    message = malloc(packetLength);
    memset(message, 0, packetLength);
    message[0] = 0x80; //magic
    message[1] = 0x01; //opcode
    message[2] = (keylen >> 8) & 0xFF;
    message[3] = keylen & 0xFF;
    message[4] = 0x08; // extra length
    message[8] = (bodyLenght & 0xFF000000) >> 24; // total body
    message[9] = (bodyLenght & 0xFF0000) >> 16; // total body
    message[10] = (bodyLenght & 0xFF00) >> 8; // total body
    message[11] = (bodyLenght & 0xFF); // total body
    message[28] = (expire & 0xFF000000) >> 24; // total body
    message[29] = (expire & 0xFF0000) >> 16; // total body
    message[30] = (expire & 0xFF00) >> 8; // total body
    message[31] = (expire & 0xFF); // total body
    memcpy(message + 32, key, keylen);
    memcpy(message + 32 + keylen, value, vallen);

    if (send(memSock, message, packetLength, 0) != packetLength) {
      wlog("Memcached: Mismatch in number of sent bytes");
    }

    resp = malloc(headerLength);
    memset(resp, 0, headerLength);

    received = recv(memSock, resp, headerLength, 0);
    if (received < 0) {
        wlog("Failed to receive responce from memcache");
        res = -1;
    }
    if (received == headerLength) {
        if (resp[0] != 0x81) res = -1002;
        if (message[1] != resp[1]) res = -1003;
        if (message[2] != resp[2]) res = -1004;
        int status;
        status = resp[6] * 0xFF + resp[7];
        if (status != 0) res = status;
    }
    free (message);
    free (resp);
    return res;
}

int memcache_get(char * key, unsigned int keylen, char * value) {
    unsigned char * message;
    unsigned char * resp;
    unsigned int headerLength, bodyLenght, packetLength;
    int received, res;
    res = 0;
    headerLength = 24;
    bodyLenght = keylen;
    packetLength = headerLength + bodyLenght; 
    
    message = malloc(packetLength);
    memset(message, 0, packetLength);
    message[0] = 0x80; //magic
    message[1] = 0x00; //opcode
    message[2] = (keylen >> 8) & 0xFF;
    message[3] = keylen & 0xFF;
    message[4] = 0x00; // extra length
    message[8] = (bodyLenght & 0xFF000000) >> 24; // total body
    message[9] = (bodyLenght & 0xFF0000) >> 16; // total body
    message[10] = (bodyLenght & 0xFF00) >> 8; // total body
    message[11] = (bodyLenght & 0xFF); // total body
    
    memcpy(message + headerLength, key, keylen);
    
    if (send(memSock, message, packetLength, 0) != packetLength) {
      wlog("Memcached: Mismatch in number of sent bytes");
    }

    resp = malloc(headerLength);
    memset(resp, 0, headerLength);

    received = recv(memSock, resp, headerLength, 0);
    if (received < 0) {
        wlog("Failed to receive responce from memcache");
        res = -1;
    }
    int i;
    if (received == headerLength) {
        if (resp[0] != 0x81) res = -1002;
        if (message[1] != resp[1]) res = -1003;
        if (message[2] != resp[2]) res = -1004;
        unsigned int status = 0;
        status = (resp[6] << 8) + resp[7];
        unsigned long vallen;
        vallen = (resp[8] << 24) + (resp[9] << 16) + (resp[10] << 8) + resp[11];
        if (status != 0) {
            res = 0 - 2000 - status;            
        }   
        if (vallen > 0) {
                memset(value, vallen, 0);
                char flags[4];
                received = recv(memSock, flags, 4, 0);
                if (received < 4) {
                    wlog("Failed to receive flags from memcache");
                    res = -1;
                } else {
                    received = recv(memSock, value, vallen - 4, 0);
                    if (received < vallen - 4) {
                        wlog("Failed to receive value from memcache");
                        res = -1;
                    } else {
                        if (res >= 0) {
                            res = vallen - 4;
                            value[vallen - 4] = 0;
                        }
                    }
                }
        } else {
                res = -999; // not found
        }
        
    }
    free (message);
    free (resp);
    return res;
}