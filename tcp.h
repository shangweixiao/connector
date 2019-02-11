#ifndef __TCP_H__
#define __TCP_H__

#include "connector.h"

// -1 soceket error,0 success
int tcp_send(IN const socket_t socket,IN char *buffer,IN int length);
int tcp_recv(IN socket_t socket,IN char *buffer, int length);
void tcp_set_socket_opt(IN socket_t sock,IN int async);
socket_t tcp_create_server_socket(IN int port,IN int async);
socket_t tcp_create_server_socket6(IN int port,IN int async);
socket_t tcp_create_server_socket_ex(IN int port,IN int async,char *ip);
int tcp_recv_select(IN socket_t socket);
int tcp_can_send(socket_t sck, int millis);
int tcp_is_socket_connected(IN socket_t socket);
// 获取socket对端的IP
int tcp_get_peer_name(IN socket_t socket,OUT char ip[IP_ADDRESS_STR_LENGTH]);
#endif