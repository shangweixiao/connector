#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/resource.h>

#include "connector.h"
#include "log.h"
#include "tools.h"
#include "threadpool.h"
#include "client_edm.h"

static int g_is_service = 1;

void *connector(void *param);

int g_edm_port = 12308;

volatile static int g_connector_quit;
void connector_quit(IN int sig)
{
    DBG_OUT("connector quit!\n");
    tools_uninit();
    threadpool_uninit();
	
    g_connector_quit = 1;
}

static int connector_init_config_file(void)
{
	FILE *f;
	char buf[256]={0},*p;
	int port;

	f = fopen(CONFIG_FILE,"r");
	if(NULL == f)
	{
		return RES_FAILURE;
	}
	rewind(f);

	memset(buf,0,sizeof(buf));
	while(NULL != fgets(buf,sizeof(buf),f))
	{
		p = strstr(buf,"=");
		if(NULL == p)
		{
			continue;
		}

		p += 1; // Skip '='
		if(buf == strstr(buf,"edm_port"))
		{
			port = atoi(p);
			if((port>1024) && (port<65536))
			{
				g_edm_port = port;
			}
		}
		else
		{
			continue;
		}
	}
	fclose(f);
	return RES_SUCCESS;
}


static int connector_set_socket_limit(uint32_t filemax)
{
    int ret;
    struct rlimit rl;

    ret = getrlimit(RLIMIT_NOFILE, &rl);
    if (ret != 0) 
    {
        fprintf(stderr,"Unable to read open file limit: (%d, %s)\n",errno, strerror(errno));
        return RES_FAILURE;
    }

    fprintf(stderr,"Limit was %d (max %d)\n",(int) rl.rlim_cur, (int) rl.rlim_max);

    if (rl.rlim_cur >= filemax) 
    {
        fprintf(stderr,"unchanged\n");
        return RES_SUCCESS;
    }
    else
    {
        fprintf(stderr,"setting to %d\n", filemax);
    }

    rl.rlim_cur = rl.rlim_max = filemax;
    ret = setrlimit(RLIMIT_NOFILE, &rl);
    if (ret != 0)
    {
        fprintf(stderr,"Unable to set open file limit: (%d, %s)\n",errno, strerror(errno));
        return RES_FAILURE;
    }

    ret = getrlimit(RLIMIT_NOFILE, &rl);
    if (ret != 0)
    {
        fprintf(stderr,"Unable to read _NEW_ open file limit: (%d, %s)\n",errno, strerror(errno));
        return RES_FAILURE;
    }

    if (rl.rlim_cur < filemax)
    {
        fprintf(stderr,"Failed to set open file limit: ""limit is %d, expected %d\n", (int) rl.rlim_cur, filemax);
        return RES_FAILURE;
    }

    return RES_SUCCESS;
}

void *connector(void *param)
{
    int res;
    char *exec_path;
    int seperator;
    char *p;

    exec_path = (char*)tools_malloc(4096);
    if(NULL == exec_path)
    {
        return NULL;
    }
    memset(exec_path,0,4096);
    res = readlink( "/proc/self/exe",exec_path,4096 );

    if((0 == res) || (4096 < res))
    {
        return NULL;
    }

    {
        sigset_t signal_mask;
        sigemptyset (&signal_mask);
        sigaddset (&signal_mask, SIGPIPE);
        int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
        if (rc != 0)
        {
            fprintf(stderr,"block sigpipe error\n");
        }
    }
    chdir(exec_path);
    tools_free(exec_path);

     //signal(SIGINT, gateway_quit); // ctrl+c
     connector_set_socket_limit(65535);

     // 调整本进程的优先级到最高
     setpriority(PRIO_PROCESS,getpid(),-20);

    res = log_init();
    if(RES_FAILURE == res)
    {
        return NULL;
    }
    DBG_OUT("##################################\n");
    DBG_OUT("          Connector Start!         \n");
    DBG_OUT("            "VERSION"          \n");
    DBG_OUT(" Build time:"__DATE__" "__TIME__ " \n");
    DBG_OUT("##################################\n");

    connector_init_config_file();

	res = threadpool_init();
	if(RES_FAILURE == res)
	{
		DBG_OUT("User thread pool init error\n");
		return NULL;
	}

    res = tools_init();
    if(RES_FAILURE == res)
    {
        DBG_OUT("Tools module init error\n");
        return NULL;
    }

    res = client_edm_init();
    if(RES_FAILURE == res)
    {
        DBG_OUT("Tools module init error\n");
        return NULL;
    }
	
    while (1)
    {
        sleep(10);
        if(g_connector_quit)
        {
            break;
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{

    if(2==argc && (0==strncmp("--noservice",argv[1],11)))
    {
        g_is_service = 0;
    }


    if(g_is_service)
    {
        daemon(1,0);
    }
    connector(NULL);

    return 0;
}
