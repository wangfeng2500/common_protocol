/*
 * socket_listen.cpp
 *
 *  Created on: 2016-2-2
 *      Author: wangfeng
 */

#include "socket_listen.h"

Socket_Listen::Socket_Listen()
{

}

Socket_Listen::~Socket_Listen()
{

}

int Socket_Listen::Create(const char* bind_ip,short bind_port)
{
	int listenfd = socket(AF_INET,SOCK_STREAM,0);
	int ret = 0;

	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int flag = 1;
	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(ret)
	{
		close(listenfd);
		API_LOG_DEBUG(LM_ERROR,"setsockopt failed, ret:%d, errno:%d", ret, errno);
		return -1;
	}

	ret = bind(listenfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret)
	{
		close(listenfd);
		API_LOG_DEBUG(LM_ERROR,"bind error: %s",strerror(errno));
		return -1;
	}

	flag = fcntl(listenfd, F_GETFL);
	if ( fcntl(listenfd, F_SETFL, O_NONBLOCK | flag) < 0 )
	{
		close(listenfd);
		API_LOG_DEBUG(LM_ERROR,"HandleConnect set noblock socket:%d error:%s", listenfd,strerror(errno));
		return -1;
	}

	if(listen(listenfd, 1024)<0)
	{
		close(listenfd);
		API_LOG_DEBUG(LM_ERROR,"CreateListen listen fd:%d err:%s", listenfd,strerror(errno));
		return -1;
	}
	_sock_fd = listenfd;
	return 0;
}

int Socket_Listen::OnRecv()
{
	API_LOG_DEBUG(LM_DEBUG,"enter into CSockListen OnRecv\n");
	int length = sizeof(struct sockaddr_in);

	/*
	 * 此种方式处理连接请求，listen要用LT模式，不要用ET模式，否则accept到的连接可能少于请求的连接
	 */
	int connfd = accept(_sock_fd,(struct sockaddr *)&client_addr,(socklen_t*)&length);
	if ( connfd <= 0 )
	{
		// 多进程情况下，会争着accept，所以导致有些报错
		if (connfd != -1)
		{
			API_LOG_DEBUG(LM_ERROR,"netlisten accept rtn:%d error:%s\n", connfd, strerror(errno));
		}
		return -1;
	}

	int flag = fcntl (connfd, F_GETFL);
	if ( fcntl (connfd, F_SETFL, O_NONBLOCK | flag) < 0 )
	{
		API_LOG_DEBUG(LM_ERROR,"HandleConnect set noblock socket:%d error:%s\n", connfd,strerror(errno));
		close(connfd);
		return -1;
	}

	if (AllocConnfd(connfd) < 0 )
	{
		API_LOG_DEBUG(LM_ERROR,"CSockListen alloc proxy connfd error\n");
		close(connfd);
		return -1;
	}

	/*
	 * 若listenfd为ET模式，则要如此处理请求

	while((connfd = accept(_sock_fd,(struct sockaddr *)&client_addr,(socklen_t*)&length)) > 0)
	{
		int flag = fcntl (connfd, F_GETFL);
		if ( fcntl (connfd, F_SETFL, O_NONBLOCK | flag) < 0 )
		{
			API_LOG_DEBUG(LM_ERROR,"HandleConnect set noblock socket:%d error:%s\n", connfd,strerror(errno));
			close(connfd);
			return -1;
		}

		if (AllocConnfd(connfd) < 0 )
		{
			API_LOG_DEBUG(LM_ERROR,"CSockListen alloc proxy connfd error\n");
			close(connfd);
			return -1;
		}
	}
	*/

	return 0;
}

int Socket_Listen::OnSend()
{
	return 0;
}

int Socket_Listen::OnClose()
{
	return 0;
}

int Socket_Listen::OnError()
{
	return 0;
}

int Socket_Listen::AllocConnfd(int connfd)
{
	Socket_Connect * socket_connect = new Socket_Connect();
	if (!socket_connect )
	{
		API_LOG_DEBUG(LM_ERROR,"CSockListen alloc CSockClient error:%s",strerror(errno));
		return -1;
	}
	static int sequence = 0;
	unsigned clientip = client_addr.sin_addr.s_addr;
	char* ip_str;
	struct in_addr _in;
	_in.s_addr = clientip;
	ip_str = inet_ntoa(_in);
	API_LOG_DEBUG(LM_DEBUG,"alloc connfd:%d, ip is %s",connfd, ip_str);
	socket_connect->setClientIP(ip_str);
	socket_connect->_sock_fd = connfd;
	socket_connect->AttachEpoller(_epoller);
	socket_connect->SetEvent(FD_RECV|FD_CLOSE|FD_ERROR);
	socket_connect->SetSockListen(this);
	socket_connect->setSqeuence(++sequence);
	return 0;
}
