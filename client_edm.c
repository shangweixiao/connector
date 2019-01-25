#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#include "connector.h"
#include "log.h"
#include "tcp.h"
#include "threadpool.h"
#include "tools.h"

#define CONNECTOR_CFG_FILE "/etc/connector/connector.cfg"

typedef struct __connector_cfg__
{
	char key[16];
	char val[256];
}connect_cfg_t;

typedef struct __client_socket__
{
    socket_t socket;
    char edm_ip[IP_ADDRESS_STR_LENGTH];
} client_socket_t;

#define HAPROXY_CFG_FILE "/etc/haproxy/haproxy.cfg"
static int edm_write_haproxy_file(char edm_ip[IP_ADDRESS_STR_LENGTH])
{
	FILE *fp;
	char *buf,*p;
	int size;

	fp= fopen(HAPROXY_CFG_FILE,"rt+");
	if(NULL == fp)
	{
		DBG_OUT("ERROR:Open haproxy.cfg file\n");
		return RES_FAILURE;
	}
	rewind(fp);
	
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	buf = tools_malloc(size+4);
	rewind(fp);
	fread(buf,1,size,fp);
	fclose(fp);
	fp = NULL;
	if(NULL == strstr(buf,edm_ip))
	{
		char cmd[256]={0};
		sprintf(cmd,"sed -i 's/.*acl.*$/& %s/g' %s",edm_ip,HAPROXY_CFG_FILE);
		system(cmd);

		system("service haproxy reload");
	}

	tools_free(buf);
	return RES_SUCCESS;
}

static int edm_exec_cmd(char cmd[256])
{
	system(cmd);

	return RES_SUCCESS;
}
static char g_password[3][36] = {0};
void *client_edm_generate_password(IN void *param)
{
	time_t t;
	struct tm *ptm;
	
    char buf[64] = {0};
    int fd[2];
    int backfd;
	pipe(fd);
	char cmd[64] = {0};
	while(1)
	{
		t = time(NULL);
		ptm = gmtime(&t);

		backfd=dup(STDOUT_FILENO);
		dup2(fd[1],STDOUT_FILENO);

		sprintf(cmd,"echo -n %4d%02d%02d%02d%02d%s | md5sum",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,param);
		system(cmd);
		read(fd[0],buf,sizeof(buf));

		dup2(backfd,STDOUT_FILENO);

		memcpy(&g_password[ptm->tm_min%3][0],buf,strlen(buf));
		memset(buf,0,sizeof(buf));

		sleep(60);
	}

}

void *client_edm_process_thread(IN void *param)
{
    client_socket_t *client = (client_socket_t*)param;
    int recv_bytes;
    char head[64] = {0};

    while(1)
    {
        // 接收消息
        recv_bytes = tcp_recv(client->socket,head,sizeof(head));
        if(RES_FAILURE == recv_bytes)
        {
            DBG_OUT("socket recive head error! Quit!\n");
            break;
        }

		if(NULL != strstr(head,"GET")  && NULL != strstr(head,"HTTP"))
		{
			DBG_OUT("IT IS NOT A HTTP GET REQUEST! ip=%s\n",client->edm_ip);
			break;
		}

		if(NULL != strstr(head,"smile"))
		{
			edm_write_haproxy_file(client->edm_ip);
			DBG_OUT("edm_write_haproxy_file! ip=%s\n",client->edm_ip);
		}
		else if(NULL != strstr(head,"harestart"))
		{
			edm_exec_cmd("systemctl restart haproxy.service");
			DBG_OUT("systemctl restart haproxy.service! ip=%s\n",client->edm_ip);
		}
		else if(NULL != strstr(head,"ssrestart"))
		{
			edm_exec_cmd("systemctl restart shadowsocks-libev.service");
			DBG_OUT("systemctl restart shadowsocks-libev.service! ip=%s\n",client->edm_ip);
		}
		else if(NULL != strstr(head,"sysrestart"))
		{
			edm_exec_cmd("reboot");
			DBG_OUT("connector start! ip=%s\n",client->edm_ip);
		}
		else if(NULL != strstr(head,"clearip"))
		{
			edm_exec_cmd("sed -i 's/.*acl.*$/\\tacl allow_ip src 127\\.0\\.0\\.1/g' /etc/haproxy/haproxy.cfg");
			DBG_OUT("clear haproxy ips! ip=%s\n",client->edm_ip);
		}
		break;
    }

    DBG_OUT("EDM connection closed and release resource.Socket=%d\n",client->socket);
	CLOSE_SOCKET(client->socket);
	tools_free(client);
    
    pthread_exit(NULL);
}


void *client_edm_thread(IN void *param)
{
    int sin_size;
    socket_t sockfd,client_fd;
    struct sockaddr_in remote_addr,local_addr; /* 客户端地址信息 */
    unsigned char *p;
    client_socket_t *client_socket;
    int len;

    sockfd = tcp_create_server_socket(EDM_LISTEN_PORT,0);
    if(RES_FAILURE == sockfd)
    {
        DBG_OUT("Error to create client edm main socket server\n");
        exit(1);
    }

    while(1)
    {
        sin_size = sizeof(struct sockaddr_in);

        if ((client_fd = accept(sockfd, (struct sockaddr *)&remote_addr,&sin_size)) == -1)
        {
            continue;
        }

        client_socket = (client_socket_t*)tools_malloc(sizeof(client_socket_t));

        tcp_set_socket_opt(client_fd,0);

        p = (unsigned char *)inet_ntoa(remote_addr.sin_addr);
        sprintf((char*)client_socket->edm_ip,"%s",p);

        client_socket->socket = client_fd;

        DBG_OUT("Recive a edm connection.Socket=%d,IP=%s\n",client_fd,client_socket->edm_ip);
        threadpool_add_task(client_edm_process_thread,client_socket);
    }

    pthread_exit(NULL);
}

void read_connector_cfg_file(char *file)
{
	
}

int client_edm_init()
{
	read_connector_cfg_file(CONNECTOR_CFG_FILE);
	threadpool_add_task(client_edm_thread,NULL);
}

int client_edm_uninit()
{
	return 0;
}

