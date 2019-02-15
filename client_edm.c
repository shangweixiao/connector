#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <openssl/md5.h>

#include "connector.h"
#include "log.h"
#include "tcp.h"
#include "threadpool.h"
#include "tools.h"

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

#define MD5_VAL_BYTES  16
#define MD5_STRING_LENGTH 32
static char g_password[3][MD5_STRING_LENGTH + 1] = {0};
void *client_edm_generate_password(IN void *param)
{
	time_t t;
	struct tm *ptm;
	char d[MD5_VAL_BYTES] = {0};
	MD5_CTX x;
	char password[64] = {0};
	int length;
	char md5[MD5_STRING_LENGTH] = {0};
	int i;

	while(1)
	{
		t = time(NULL);
		ptm = gmtime(&t);

		length = snprintf(password,sizeof(password),"%4d%02d%02d%02d%02d%s",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,param);
		MD5_Init(&x);
		MD5_Update(&x,password,length);
		MD5_Final(d,&x);
		
		for(i=0;i<MD5_VAL_BYTES;i++)
		{
			sprintf(&md5[i*2],"%02X",(unsigned char)d[i]);
		}

		memcpy(&g_password[ptm->tm_min%3][0],md5,MD5_STRING_LENGTH);
		memset(md5,0,sizeof(md5));
		
		//printf("%s\n",&g_password[ptm->tm_min%3][0]);
		sleep(60);
	}
}

void *client_edm_process_thread(IN void *param)
{
    client_socket_t *client = (client_socket_t*)param;
    int recv_bytes;
    char head[1024] = {0};

    while(1)
    {
        // 接收消息
        memset(&head[0],0,sizeof(head));
        recv_bytes = tcp_recv(client->socket,head,sizeof(head)-1);
        if(RES_FAILURE == recv_bytes)
        {
            DBG_OUT("socket recive head error! ip=%s\n",client->edm_ip);
            break;
        }

		if(NULL == strstr(head,"GET")  || NULL == strstr(head,"HTTP"))
		{
			DBG_OUT("IT IS NOT A HTTP GET REQUEST! ip=%s head = %s\n",client->edm_ip,head);
			break;
		}

		DBG_OUT("\n%s\n",head);

		if(NULL != strstr(head,"harestart"))
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
			system("service haproxy reload");
			DBG_OUT("clear haproxy ips! ip=%s\n",client->edm_ip);
		}
		else
		{
			int i;
			for(i=0;i<3;i++)
			{
				if(MD5_STRING_LENGTH == strlen(g_password[i]) && NULL != strstr(head,&g_password[i][0]))
				{
					edm_write_haproxy_file(client->edm_ip);
					memset(&g_password[i][0],0,sizeof(g_password[i]));
					DBG_OUT("edm_write_haproxy_file! ip=%s\n",client->edm_ip);
					break;
				}
			}
		}
		break;
    }

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

        threadpool_add_task(client_edm_process_thread,client_socket);
    }

    pthread_exit(NULL);
}

void *client_edm_ipv6_thread(IN void *param)
{
    int sin_size;
    socket_t sockfd,client_fd;
    struct sockaddr_in6 remote_addr,local_addr; /* 客户端地址信息 */
    client_socket_t *client_socket;
    int len;

    sockfd = tcp_create_server_socket6(EDM_LISTEN_PORT,0);
    if(RES_FAILURE == sockfd)
    {
        DBG_OUT("Error to create client edm main socket server\n");
        exit(1);
    }

    while(1)
    {
        sin_size = sizeof(struct sockaddr_in6);

        if ((client_fd = accept(sockfd, (struct sockaddr *)&remote_addr,&sin_size)) == -1)
        {
            continue;
        }

        client_socket = (client_socket_t*)tools_malloc(sizeof(client_socket_t));

        tcp_set_socket_opt(client_fd,0);

        inet_ntop(AF_INET6,&remote_addr.sin6_addr,(char*)client_socket->edm_ip,INET6_ADDRSTRLEN);

        client_socket->socket = client_fd;

        threadpool_add_task(client_edm_process_thread,client_socket);
    }

    pthread_exit(NULL);
}

int client_edm_init()
{

	threadpool_add_task(client_edm_generate_password,"0029787");
	threadpool_add_task(client_edm_thread,NULL);
	threadpool_add_task(client_edm_ipv6_thread,NULL);
}

int client_edm_uninit()
{
	return 0;
}

