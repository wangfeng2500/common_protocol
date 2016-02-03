/*
 * socket_listen.h
 *
 *  Created on: 2016-2-2
 *      Author: wangfeng
 */

#ifndef SOCKET_LISTEN_H_
#define SOCKET_LISTEN_H_

#include "epoller.h"
#include "log.h"
#include "socket_connect.h"

using namespace CGI_LOG;

#ifndef SERVER_PORT
#define SERVER_PORT 9999
#endif

class Socket_Listen:public CEpollSocket
{
public:
	Socket_Listen();
	~Socket_Listen();

	int Create(const char* bind_ip,short bind_port);

	virtual int OnRecv();
	virtual int OnSend();
	virtual int OnClose();
	virtual int OnError();
private:
	int AllocConnfd(int connfd);
	struct sockaddr_in client_addr;
};



#endif /* SOCKET_LISTEN_H_ */
