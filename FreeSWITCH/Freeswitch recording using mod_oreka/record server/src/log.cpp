#include "global.h"
#include <stdarg.h>
#include <sys/time.h> // for clock_gettime()

// only database server use
log_level g_log_level = info;
pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
const char *g_level_msg[] = {
    "[ debug  ]",
    "[  info  ]",
    "[ notice ]",
    "[warning ]",
    "[  err   ]",
    "[critical]",
    "[  file  ]",
    ""
};

void myLogToFile(log_level level, const char *szTime, const char *szPath, const char * p)
{

    if(level < g_log_level) return;
    if(level > file_only) return;

    pthread_mutex_lock(&g_log_mutex);

    FILE *fhLog;
    fhLog = fopen(szPath, "a+");
    if(NULL == fhLog){
        //error		
        fprintf(stderr,  "Log File Path[%s] Not Found\n" , szPath);	
    pthread_mutex_unlock(&g_log_mutex);
        return;
    }

    if(level != file_only) fprintf(stderr, "%s%s\n", g_level_msg[level], p);

    fwrite(szTime, 1, strlen(szTime), fhLog);
    fwrite(p, 1, strlen(p), fhLog);
    fwrite("\n", 1, 1, fhLog);
    fclose(fhLog);
    pthread_mutex_unlock(&g_log_mutex);
}

void LogToFile(log_level level, const char * szFormat, ...)
{
    char szPath[256];
    char p[2048], szTime[128];
    struct tm *ntime;
    timeval curTime;
    va_list args;

    if(level < g_log_level) return;
    if(level > file_only) return;

    gettimeofday(&curTime, NULL);
    ntime = localtime(&curTime.tv_sec);
    sprintf(szTime, "%s [%d/%02d/%02d-%02d:%02d:%02d.%03d]:", g_level_msg[level], ntime->tm_year + 1900, ntime->tm_mon + 1, ntime->tm_mday, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, (int)(curTime.tv_usec/ 1000));
    sprintf(szPath, "%s/epoll_%d%02d%02d.log",  LOG_PATH ,  ntime->tm_year + 1900, ntime->tm_mon + 1, ntime->tm_mday);
    va_start(args, szFormat);
    vsnprintf (p, sizeof(p), szFormat, args);
    va_end(args) ;

    myLogToFile(level, szTime, szPath, p);
}

void LogToFileEx(log_level level, const char *filename, const char * szFormat, ...)
{
    char szPath[256];
    char p[2048], szTime[128];
    struct tm *ntime;
    timeval curTime;
    va_list args;

    if(level < g_log_level) return;
    if(level > file_only) return;

    gettimeofday(&curTime, NULL);
    ntime = localtime(&curTime.tv_sec);
    sprintf(szTime, "%s [%d/%02d/%02d-%02d:%02d:%02d.%03d]:", g_level_msg[level], ntime->tm_year + 1900, ntime->tm_mon + 1, ntime->tm_mday, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, (int)(curTime.tv_usec/ 1000));
    sprintf(szPath, "%s/%s_%d%02d%02d.log",  LOG_PATH , filename, ntime->tm_year + 1900, ntime->tm_mon + 1, ntime->tm_mday);
    va_start(args, szFormat);
    vsnprintf (p, sizeof(p), szFormat, args);
    va_end(args) ;

    myLogToFile(level, szTime, szPath, p);
}


void set_log_level(log_level val)
{
    g_log_level = val;
}
