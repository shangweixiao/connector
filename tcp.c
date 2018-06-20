#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <errno.h>
#include <sys/epoll.h> 
#include <fcntl.h>

#include "connector.h"
#include "log.h"


// 5/18/2018 18:48:56
#define TCP_STRERROR strerror(errno)
#define TCP_BLOCKS (errno == EWOULDBLOCK)
#define TCP_ERRNO errno

int tcp_is_socket_connected(IN socket_t socket) 
{
    int len;
    int res,val;
    struct sockaddr_in sa;

    len = sizeof(sa);
    res = getpeername(socket, (struct sockaddr *)&sa, &len);
    if(0 != res)
    {
        DBG_OUT("Socket(%d) break.(getpeername)Error No:%d\n",socket,TCP_ERRNO);
        return RES_FALSE;
    }

    len=sizeof(int);
    res = getsockopt(socket,SOL_SOCKET, SO_ERROR,(char *) &val, &len);
    if((-1 == res) || (0 != val))
    {
        DBG_OUT("Socket(%d) break.(getsockopt:SOL_SOCKET)Error No:%d\n",socket,TCP_ERRNO);
        return RES_FALSE;
    }

    {
        struct tcp_info info;
        int length=sizeof(info);
        getsockopt(socket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&length); 
        if((info.tcpi_state!=TCP_ESTABLISHED)) 
        {
            DBG_OUT("Socket(%d) break.(getsockopt:IPPROTO_TCP)Error No:%d\n",socket,TCP_ERRNO);
            return RES_FALSE; 
        }
    } 

    return RES_TRUE;
}

int tcp_can_send(socket_t sck, int millis)
{
    fd_set wfds;
    struct timeval time;
    int sel_count;

    time.tv_sec = millis / 1000;
    time.tv_usec = (millis * 1000) % 1000000;
    FD_ZERO(&wfds);
    FD_SET(sck, &wfds);
    sel_count = select(sck + 1, 0, &wfds, 0, &time);
    if (sel_count > 0)
    {
        return RES_TRUE;
    }
    return RES_FALSE;
}

int tcp_send(IN const socket_t socket,IN char *buffer,IN int length)
{
    int sent, total = 0;
    int tmout=200; // 200次，每次100ms，共20s

    assert(NULL != buffer);

    if(0 == length)
    {
        DBG_OUT("ERROR:Send buffer is NULL.Socket=%d\n",socket);
        return RES_FAILURE;
    }

    while (total < length)
    {
		if(RES_FALSE == tcp_can_send(socket, 100)) // 返回超时
		{
			if(RES_FALSE == tcp_is_socket_connected(socket)) // socket已经断开
			{
				DBG_OUT("ERROR:Socket send error:socket closed!Socket=%d\n",socket);
				return RES_FAILURE;
			}

			tmout -= 1;
			if(tmout <=0) // socket 发送超时
			{
				DBG_OUT("ERROR:Socket send error:socket timeout!Socket=%d\n",socket);
				return RES_FAILURE;
			}
			DBG_OUT("socket(%d) can't send data at this time.time out=%d\n",socket,tmout);
		}
		else // send data
		{
			sent = send(socket, buffer + total, length - total, 0);
			if (sent <= 0)
			{
				if (sent == -1 && TCP_BLOCKS)
				{
					sent = 0;
				}
				else
				{
					DBG_OUT("ERROR:Socket(%d) send: %s,Error No:%d\n",socket,TCP_STRERROR,TCP_ERRNO);
					return RES_FAILURE;
				}
			}
			else
			{
				tmout=200;
				total += sent;
			}
		}
    }
    return RES_SUCCESS;
}

/* Returns 0 success, -1 error */
int tcp_recv_select(IN socket_t socket)
{
    fd_set rfds, wfds;
    struct timeval tv;
    int num = 12;

    //先检测一下socket是否正确
    if(!tcp_is_socket_connected(socket))
    {
        DBG_OUT("check socket connect error\n");
        return -1;
    }

    while (RES_TRUE)
    {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(socket, &rfds);

        /* default timeout */
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        switch (select(socket+1, &rfds, NULL, NULL, &tv))
        {
        case -1:
            DBG_OUT("ERROR:Socket(%d) select: %s\n",socket,strerror(errno));
            return -1;

        case 0: // timeout
            if(!tcp_is_socket_connected(socket))
            {
                if(0 >= num)
                {
                    DBG_OUT("ERROR:Socket(%d) timeout!\n",socket);
                    return -1;
                }
                num -= 1;
            }
            else
            {
                num = 12;
            }
            continue;
        }

        if (FD_ISSET(socket, &rfds))
        {
            return 0;
        }
    }
}
// -1 soceket close,0 success
int tcp_recv(IN socket_t socket,IN char *buffer, int length)
{
    int rcvd = 0;

    while (length > 0)
    {
        if (-1 == tcp_recv_select(socket))
        {
            /* socket error */
            DBG_OUT("ERROR:Socket(%d) recv: %s\n",socket,TCP_STRERROR);
            return RES_FAILURE;
        }

        rcvd = recv(socket, buffer, length, 0);
        if (rcvd < 0)
        {
            if (rcvd == -1 && TCP_BLOCKS)
            {
                rcvd = 0;
            }
            else
            {
                DBG_OUT("ERROR:Socket(%d) recv: %s,Error No:%d\n",socket,TCP_STRERROR,TCP_ERRNO);
                return RES_FAILURE;
            }
        }
        else if (rcvd == 0)
        {
            DBG_OUT("ERROR:Socket(%d) connection closed.Error No:%d\n",socket,TCP_ERRNO);
            return RES_FAILURE;
        }

        buffer += rcvd;
        length -= rcvd;
    }

    return RES_SUCCESS;
}

static void tcp_set_nonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL,0);
    if(opts<0)
    {
        return;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
        DBG_OUT("Set socket error:%s,errno=%d\n",strerror(errno),errno);

        return;
    }
}

void tcp_set_socket_opt(IN socket_t sock,IN int async)
{
    int val = 1;
    int len=sizeof(val);
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;

    setsockopt(sock,SOL_SOCKET,SO_LINGER,(char *)&so_linger,sizeof so_linger);
    setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char *) &val,sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&val,sizeof(int));

    /* receive buffer must be a least 16 K */
    val = 1024*64;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&val,sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&val,sizeof(int));
    {
        /* Set keepalive parameters */
        int tcp_keepalive_time = 8;
        int tcp_keepalive_interval = 8;
        int tcp_keepalive_probes = 8;
        int optlen;
		struct timeval t;

        optlen = sizeof(tcp_keepalive_time);
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &tcp_keepalive_time, optlen);
        optlen = sizeof(tcp_keepalive_interval);
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &tcp_keepalive_interval, optlen);
        optlen = sizeof(tcp_keepalive_probes);
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &tcp_keepalive_probes, optlen);

		t.tv_sec = 20;	//设置20秒
		t.tv_usec = 0;
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t));
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
    }

    if(async)
    {
        tcp_set_nonblocking(sock);
    }
}

socket_t tcp_create_server_socket(IN int port,IN int async)
{
    socket_t sockfd;
    struct sockaddr_in my_addr; /* 本机地址信息 */
    int bReuseaddr=1;

    // 主线程，监听命令端口
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        DBG_OUT("Error creating socket!\n");
        return(RES_FAILURE);
    }

    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(int));
    tcp_set_socket_opt(sockfd,async);

    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero),0,8);
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        DBG_OUT("Error binding socket!\n");
        return(RES_FAILURE);
    }
    if (listen(sockfd, MAX_CONNECTION) == -1)
    {
        DBG_OUT("Error listening socket!\n");
        return(RES_FAILURE);
    }
    return sockfd;
}
socket_t tcp_create_server_socket_ex(IN int port,IN int async,char *ip)
{
    socket_t sockfd;
    struct sockaddr_in my_addr; /* 本机地址信息 */
    int bReuseaddr=1;

     // 主线程，监听命令端口
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        DBG_OUT("Error creating socket!\n");
        return(RES_FAILURE);
    }

    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(int));
    tcp_set_socket_opt(sockfd,async);

    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(port);
    my_addr.sin_addr.s_addr = inet_addr(ip);
    memset(&(my_addr.sin_zero),0,8);
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        DBG_OUT("Error binding socket!\n");
        return(RES_FAILURE);
    }
    if (listen(sockfd, MAX_CONNECTION) == -1)
    {
        DBG_OUT("Error listening socket!\n");
        return(RES_FAILURE);
    }
    return sockfd;
}

int tcp_get_peer_name(IN socket_t socket,OUT char ip[IP_ADDRESS_STR_LENGTH])
{
	int len;
	int res,val;
	struct sockaddr_in sa;

	len = sizeof(sa);
	res = getpeername(socket, (struct sockaddr *)&sa, &len);
	if(0 != res)
	{
		DBG_OUT("Socket(%d) break.(getpeername)Error No:%d\n",socket,TCP_ERRNO);
		return RES_FAILURE;
	}

	if(inet_ntop(AF_INET,&sa.sin_addr,ip,IP_ADDRESS_STR_LENGTH)==NULL) /*地址由二进制数转换为点分十进制*/
	{
		DBG_OUT("fail to convert peer name\n");
		return RES_FAILURE;
	}
	return RES_SUCCESS;
}