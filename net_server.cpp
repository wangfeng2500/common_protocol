/*
 * net_server.cpp
 *
 *  Created on: 2016年1月28日
 *      Author: fenngwang
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <string>
#include "NetDefine.h"
#include "log.h"
#include "netserver.pb.h"
#include "message_process.h"

using namespace std;
using namespace CGI_LOG;

#define SERVER_PORT 9999

int main()
{
	CGI_Log_Init(__FILE__);
	int listenfd = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int flag = 1;
	int ret = 0;
	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(ret)
	{
		API_LOG_DEBUG(LM_ERROR,"setsockopt failed, ret:%d, errno:%d", ret, errno);
	    exit(1);
	}

	ret = bind(listenfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret)
	{
		API_LOG_DEBUG(LM_ERROR,"bind error: %s",strerror(errno));
		return -1;
	}

	ret = listen(listenfd, 3);
	if(ret)
	{
		API_LOG_DEBUG(LM_ERROR,"listen error: %s",strerror(errno));
		return -1;
	}
	API_LOG_DEBUG(LM_TRACE, "net_server begin listen");

	socklen_t client_len;
	struct sockaddr_in client_addr;
	int connfd;
	for(;;)
	{
		client_len = sizeof(client_addr);
		connfd = accept(listenfd,(struct sockaddr*)&client_addr,&client_len);
		if(connfd)
		{
			char buffer[MaxPacketLength] = {0}; // 应用层缓冲区
			Message_Process msg_input;
			uint32_t already_len = 0; // 已经接收的长度
			while(1)
			{
				ret = recv(connfd,buffer+already_len,MaxPacketLength-already_len,0);
				if(ret < 0)
				{
					if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)  // 当做正确接收
					{
						continue;
					}
					else
					{
						API_LOG_DEBUG(LM_ERROR,"recv error. ret:%d, errno:%d", ret, errno);
						close(connfd);
						break;
					}
				}
				else if(ret == 0)  // 对方已经断掉
				{
					API_LOG_DEBUG(LM_ERROR,"recv close");
					close(connfd);
					break;
				}
				else
				{
					already_len += ret;
					API_LOG_DEBUG(LM_TRACE,"before process_buffer, already_len:%d, ret:%d", already_len, ret);
					if(msg_input.process_buffer(buffer, already_len) < 0)
					{
						API_LOG_DEBUG(LM_ERROR,"process_buffer failed");
						close(connfd);
						break;
					}
					API_LOG_DEBUG(LM_TRACE,"after process_buffer, already_len:%d", already_len);
				}
			}
		}
	}
	return 0;
}



