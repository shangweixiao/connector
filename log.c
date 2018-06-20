#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>


#include "connector.h"
#include "log.h"
#include "tools.h"

MUTEX_LOCKER g_log_mutex;

static FILE *log_file=NULL;
static char log_path[1024+32];

int log_init(void)
{
    int res;
    
    res = MUTEX_INIT(&g_log_mutex,NULL);
    if(0 != res)
    {
        return RES_FAILURE;
    }
  
    sprintf(log_path,"/var/log/%s",LOG_FILE_NAME);

    log_file = fopen(log_path,"a+");
    if(NULL == log_file)
    {
        DBG_OUT("Open the log file error!\n");
        //return RES_FAILURE;
    }
    
    return RES_SUCCESS;
}
void log_output(IN const char *str,...)
{
    va_list args;
    time_t ltime;
    struct tm *tm;

    MUTEX_LOCK(&g_log_mutex);

    if(NULL == log_file)
    {
        MUTEX_UNLOCK(&g_log_mutex);
        return;
    }

    time(&ltime);
    tm=localtime(&ltime);

    if(LOG_FILE_SIZE <= ftell(log_file))
    {
        fclose(log_file);

        log_file = fopen(log_path,"w+");
        if(NULL == log_file)
        {
            MUTEX_UNLOCK(&g_log_mutex);
            return;
        }
    }

    fprintf(log_file,"[%d/%02d/%02d,%02d:%02d:%02d]  ",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);

    va_start(args, str);
    vfprintf(log_file, str, args);
    va_end(args);

    fflush(log_file);
    MUTEX_UNLOCK(&g_log_mutex);
}

int log_uninit(void)
{
    if(NULL != log_file)
    {
        fclose(log_file);
    }


    MUTEX_UNINIT(&g_log_mutex);
    return RES_SUCCESS;
}

