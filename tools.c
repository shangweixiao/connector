#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "connector.h"
#include "log.h"
#include "tools.h"
#include "threadpool.h"


// modify bug2 5/18/2018 10:34:49
// modify bug2 5/18/2018 10:35:51
#include <uuid/uuid.h>

MUTEX_LOCKER g_tools_uid_gen;

//#define TOOLS_MALLOC_TRACE
#ifdef TOOLS_MALLOC_TRACE
#define TOOLS_MALLOC_TRACE_MAX 100000
MUTEX_LOCKER g_tools_malloc_mutex;
int g_tools_malloc_count;
static void* g_tools_malloc_pointer[TOOLS_MALLOC_TRACE_MAX]={NULL};
#endif

void *tools_malloc(int size)
{
    void *ret;
    do 
    {
        ret = calloc(1,size);
    } while(NULL == ret);

#ifdef TOOLS_MALLOC_TRACE
    {
        int i;

        MUTEX_LOCK(&g_tools_malloc_mutex);
        if(TOOLS_MALLOC_TRACE_MAX > g_tools_malloc_count)
        {
            for(i=0;i<TOOLS_MALLOC_TRACE_MAX;i++)
            {
                if(NULL == g_tools_malloc_pointer[i])
                {
                    g_tools_malloc_pointer[i] = ret;
                    break;
                }
            }
            g_tools_malloc_count += 1;
            DBG_OUT("memory alloc count = %d,pointer=%p\n",g_tools_malloc_count,ret);
        }
        else
        {
            DBG_OUT("WARNING:memory trace out! pointer=%p\n",ret);
        }
        MUTEX_UNLOCK(&g_tools_malloc_mutex);
    }
#endif
    return ret;
}
void *tools_realloc(IN void *buf,IN int new_size)
{
    void *ret;
    do 
    {
        ret = realloc(buf,new_size);
    } while(NULL == ret);
    return ret;
}
void tools_free(IN void *buf)
{
    if(NULL != buf)
    {
#ifdef TOOLS_MALLOC_TRACE
        {
            int i;
            MUTEX_LOCK(&g_tools_malloc_mutex);
            for(i=0;i<TOOLS_MALLOC_TRACE_MAX;i++)
            {
                if(g_tools_malloc_pointer[i] == buf)
                {
                    g_tools_malloc_count -= 1;
                    g_tools_malloc_pointer[i] = NULL;
                    DBG_OUT("memory alloc count = %d,pointer=%p\n",g_tools_malloc_count,buf);
                    break;
                }
            }
            if(i >= TOOLS_MALLOC_TRACE_MAX)
            {
                DBG_OUT("WARNING:Memory leak!pointer=%p\n",buf);
            }
            MUTEX_UNLOCK(&g_tools_malloc_mutex);
        }
#endif
        free(buf);
        buf = NULL;
    }
}

// BKDR Hash Function
unsigned int tools_BKDRHash(IN const char *str)
{   
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..   
    unsigned int hash = 0;
    int len = strlen(str);
    while (len)  
    {       
        hash = hash * seed + (*str++);  
        len--;
    }    
    return (hash & 0x7FFFFFFF);
}

int tools_init()
{
	MUTEX_INIT(&g_tools_uid_gen,NULL);

#ifdef TOOLS_MALLOC_TRACE
	MUTEX_INIT(&g_tools_malloc_mutex,0);
#endif
	return RES_SUCCESS;
}
int tools_uninit()
{
	MUTEX_UNINIT(&g_tools_uid_gen);

#ifdef TOOLS_MALLOC_TRACE
	MUTEX_UNINIT(&g_tools_malloc_mutex);
#endif
	return RES_SUCCESS;
}