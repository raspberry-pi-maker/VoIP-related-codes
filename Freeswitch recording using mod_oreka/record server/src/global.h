#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <list>
#include <string>
#include <time.h>
#include <cmath>

#include <osipparser2/internal.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/osip_message.h>
#include <osipparser2/osip_parser.h>
#include <osipparser2/osip_body.h>

#define LOG_PATH  "."
#define MAXEVENTS 128
#define SIP_BUFF 4096
#define MAX_CLIENT 1024
#define MAX_PATH 512
enum log_level {
    debug = 0,
    info,
    notice,
    warning,
    err,
    critical,
    file_only
};

using namespace std;

extern int g_sip_port;
extern int g_sip_sock;


extern log_level g_log_level;
void myLogToFile(log_level level, const char *szTime, const char *szPath, const char * p);
void LogToFile(log_level level, const char * szFormat, ...);
void LogToFileEx(log_level level, const char *filename, const char * szFormat, ...);
void set_log_level(log_level val);

