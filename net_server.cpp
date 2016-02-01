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

using namespace std;
using namespace CGI_LOG;

#define SERVER_PORT 9999

int process_buffer(char *buffer, uint32_t &already_len);

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
	if(ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) != 0)
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
					if(process_buffer(buffer, already_len))
					{
						API_LOG_DEBUG(LM_ERROR,"process_buffer failed", __LINE__);
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


/*
 * 参数：
 * @buffer 应用层缓冲
 * @start  起始位置
 * @length 可以处理的缓冲长度
 * return:
 * < 0 出错
 * = 0 长度不够
 * > 0 处理完毕
 */
int process_buffer(char *buffer, uint32_t &already_len)
{
	uint32_t unprocess_buffer_length = already_len; // 未处理的缓冲区长度
	int start = 0;  // 初始的读buffer的起点

	while(1)
	{
		API_LOG_DEBUG(LM_TRACE, "already_len:%d, PacketHeadLength:%d", already_len, PacketHeadLength);
		char *start_buffer = buffer+start;
		if(unprocess_buffer_length >= PacketHeadLength) // 长度比包头长
		{
			API_LOG_DEBUG(LM_TRACE, "unprocess_buffer_length:%d >= PacketHeadLength:%d", __LINE__,unprocess_buffer_length,PacketHeadLength);
			PacketHead header;
			transferBufferToPacketHead(start_buffer, header);
			if(header.uiPacketLen > MaxPacketLength-PacketHeadLength) // 客户端的包，包体太长
			{
				API_LOG_DEBUG(LM_TRACE, "header.uiPacketLen:%d > MaxPacketContentLength:%d", __LINE__, header.uiPacketLen, MaxPacketLength-PacketHeadLength);
				return -1;
			}

			if(unprocess_buffer_length < PacketHeadLength+header.uiPacketLen) // 没有接收完
			{
				API_LOG_DEBUG(LM_TRACE, "length:%d < PacketHeadLength+header.uiPacketLen:%d", __LINE__, unprocess_buffer_length, PacketHeadLength+header.uiPacketLen);
				if(start > 0)  // buffer中有完整的包，已经处理过，则要移动buffer
				{
					memmove(buffer, buffer+start, unprocess_buffer_length);
				}
				return 0;
			}
			else if(unprocess_buffer_length >= PacketHeadLength+header.uiPacketLen) // 已经有了一个完整的包
			{
				switch(header.cmd)
				{
					case CMD_GetUserName:
					{
						API_LOG_DEBUG(LM_TRACE,"recv a packet, cmd:%d", __LINE__, header.cmd);
						netserver::GetUserNameRequest request;
						if(!request.ParseFromArray(start_buffer+PacketHeadLength,header.uiPacketLen))
						{
							API_LOG_DEBUG(LM_ERROR, "ParseFromArray, cmd:%d", __LINE__, header.cmd);
							return -1;
						}
						API_LOG_DEBUG(LM_TRACE,"success, userid is %d",request.userid());
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_len = unprocess_buffer_length;
						break;
					}
					case CMD_SetUserName:
					{
						API_LOG_DEBUG(LM_TRACE," recv a packet, cmd:%d", __LINE__, header.cmd);
						netserver::SetUserNameRequest request;
						if(!request.ParseFromArray(start_buffer+PacketHeadLength,header.uiPacketLen))
						{
							API_LOG_DEBUG(LM_ERROR, "ParseFromArray, cmd:%d", __LINE__, header.cmd);
							return -1;
						}
						API_LOG_DEBUG(LM_TRACE,"success, gender is %d, name is %s, province is %s",request.gender(), request.name().c_str(), request.province().c_str());
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_len = unprocess_buffer_length;
						break;
					}
					default:
						API_LOG_DEBUG(LM_ERROR,"recv a packet, cmd:%d", __LINE__, header.cmd);
						break;
				}
			}
		}
		else // 包头都不够
		{
			API_LOG_DEBUG(LM_TRACE, "unprocess_buffer_length:%d < PacketHeadLength:%d", __LINE__,unprocess_buffer_length,PacketHeadLength);
			if(start > 0) // buffer中有完整的包，已经处理过，则要移动buffer
			{
				memmove(buffer, buffer+start, unprocess_buffer_length);
			}
			return 0;
		}
	}
	return 0;
}


