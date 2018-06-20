#ifndef __CONNECTOR_H__
#define __CONNECTOR_H__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <netinet/in.h> 
#include <net/if_arp.h>
#include <netinet/tcp.h>	/* TCP_NODELAY */
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

typedef pthread_mutex_t MUTEX_LOCKER;
#define MUTEX_INIT(locker,param) 	pthread_mutex_init(locker,param);

#define MUTEX_UNINIT(locker) \
do { \
    pthread_mutex_destroy(locker); \
}while(0)

#define MUTEX_LOCK(locker) \
do { \
    pthread_mutex_lock(locker); \
}while(0)

#define MUTEX_UNLOCK(locker) \
do { \
    pthread_mutex_unlock(locker); \
}while(0)

#define CLOSE_SOCKET(sock) \
do { \
    shutdown(sock,2); \
    close(sock); \
}while(0)
typedef int socket_t;
#define pthread_exit return
#define INVALID_SOCKET (~0)


/*================= Platform  END=================*/
#define IN
#define OUT

#define CONFIG_FILE "connector.ini" // 保存数据库的连接信息

extern int g_edm_port;
#define EDM_LISTEN_PORT g_edm_port

#define MAX_CONNECTION 500
#define IP_ADDRESS_STR_LENGTH 32
#define UUID_STR_LENGTH 40 // 12345678-1234-1234-1234-123456789012
#define MAC_ADDRESSS_LEN 32

// 版本更新记录
#define VERSION  "0.0.1"
#define VERSION_NUMBER (1)

// Default result value
#define RES_SUCCESS 0
#define RES_FAILURE -1
#define RES_TRUE 1
#define RES_FALSE 0

#define STRNCPY(d,s,l) \
do \
{ \
    (l>0) ? strncpy((d),(s),(l))[(l)-1]='\0':0; \
} while (0)

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#endif
