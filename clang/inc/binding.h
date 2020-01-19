#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>


#ifndef __BINDING__
#define __BINDING__
#define SERVER_PORT 9991

typedef struct _SocketData_t
{
  int socket;
  struct sockaddr_in addr;
} SocketData_t;


void set_socket(SocketData_t* data, int is_server)
{
  int opt;
  data->socket = socket(AF_INET, SOCK_STREAM, 0);
	if(data->socket == 0) {
    DEBUG("socket create error.");
    return;
	}
	if( setsockopt(data->socket, SOL_SOCKET,
	               SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) ) {
    DEBUG("socket option setting error.");
    return;
  }

  if(is_server) data->addr.sin_addr.s_addr = INADDR_ANY;
  data->addr.sin_family = AF_INET;
  data->addr.sin_port = htons(SERVER_PORT);

  DEBUG("Setup socket");
}


void bind_socket_to_addr(SocketData_t* data)
{
  if( bind(data->socket, (struct sockaddr*)&data->addr,
           sizeof(data->addr)) < 0) {
    DEBUG("bind failure.");
    return;
  }
  DEBUG("bind success");
  return;
}


void connect_to_server(SocketData_t* data, const char* server_addr)
{
  if(inet_pton(AF_INET, server_addr, &data->addr.sin_addr) <= 0) {
    DEBUG("Invalid address");
    return;
  }

  if(connect(data->socket, (struct sockaddr*)&data->addr,
             sizeof(data->addr)) < 0) {
    DEBUG("Connect failed to server %s", server_addr);
    return;
  }
}

#endif
