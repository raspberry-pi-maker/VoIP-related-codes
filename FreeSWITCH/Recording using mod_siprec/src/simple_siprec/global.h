#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <list>
#include <string>
#include <time.h>
#include <map>
#include <unordered_map>
#include <queue>
#include <vector>
#include <cmath>
#include <ctime>
#include <mutex>
#include <thread>
#include <cstdarg>   // C++ style
#include <chrono>
#include <condition_variable>

#include <uuid/uuid.h>
#include <netinet/in.h>   // sockaddr_in 
#include <arpa/inet.h>    // htonl, htons ...
#include <sys/socket.h>   // socket function


#include "tinyxml2.h"
#include "sip_util.h"

#define MAXEVENTS 128
#define SIP_BUFF 4096
#define MAX_CLIENT 1024
#define MAX_TENANT 16
#define MAX_PATH 512

//G.711 코덱 payload
#define ULAW_PAYLOAD 0
#define ALAW_PAYLOAD 8


typedef struct _RTP_HEADER
{
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__  
    unsigned char cc:4;          /* CSRC count */
    unsigned char extension:1;   /* header extension flag */
    unsigned char padding:1;     /* padding flag */
    unsigned char version:2;     /* protocol version */

    unsigned char type:7;          /* payload type */
    unsigned char marker:1;      /* marker bit */
    uint16_t seq:16;            /* sequence number */
  #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    unsigned char version:2;     /* protocol version */
    unsigned char padding:1;     /* padding flag */
    unsigned char extension:1;   /* header extension flag */
    unsigned char cc:4;          /* CSRC count */
    unsigned char marker:1;      /* marker bit */
    unsigned char type:7;          /* payload type */
    uint16_t seq:16;            /* sequence number */
  #else
    #error Byte Order Error
  #endif    
    uint32_t ts;                /* timestamp */
    uint32_t ssrc;              /* synchronization source */
} RTP_HEADER;


using namespace std; 

extern int g_siprec_port;
extern int g_siprec_sock;
extern std::mutex g_sip_mutex;   // pthread_mutex_t → std::mutex
extern std::queue<string> g_siprec_queue;
extern sockaddr_in g_siprec_addr;
extern socklen_t g_siprec_addr_len;
extern std::condition_variable g_cv_siprec;
