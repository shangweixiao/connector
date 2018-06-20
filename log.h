#ifndef __LOG_H__
#define __LOG_H__
#include "tools.h"
#define LOG_FILE_NAME "connector.log"
#define LOG_FILE_SIZE 100*1024*1024 // 100MB
#define LOG_INFO_LENGTH (2048)

#define DBG_OUT(args...) \
do{ \
    char b__[LOG_INFO_LENGTH+4]={0}; \
    snprintf(b__,LOG_INFO_LENGTH,args); \
    fprintf(stderr,"%u:[0x%X,%s,%d] %s",(unsigned long)time(NULL),pthread_self(),__FUNCTION__,__LINE__,b__); \
    log_output("[0x%X,%s,%d] %s",pthread_self(),__FUNCTION__,__LINE__,b__); \
}while(0)

//5/18/2018 19:00:32

///18/2018 18:58:43
//���ش���
#define DBG_FATAL DBG_OUT
//������Ϣ
#define DBG_ERROR DBG_OUT
//������Ϣ
#define DBG_WARN DBG_OUT
//һ����Ϣ
#define DBG_INFO DBG_OUT
//������Ϣ
#define DBG_MSG DBG_OUT

//dddddddddddd
//ddddsdsfd
//fix 5
int log_init(void);
void log_output(IN const char *args,...);
int log_uninit(void);
#endif
