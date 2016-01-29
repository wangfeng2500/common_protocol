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
#include "netserver.pb.h"

using namespace std;

#define SERVER_PORT 9999

int process_buffer(char *buffer, int &already_len);

int main()
{
	int listenfd = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int flag = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) != 0)
	{
		perror("setsockopt");
	    exit(1);
	}

	int ret = bind(listenfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret)
	{
		printf("bind error: %s\n",strerror(errno));
		return -1;
	}

	ret = listen(listenfd, 3);
	if(ret)
	{
		printf("listen error: %s\n",strerror(errno));
		return -1;
	}
	printf("net_server begin listen\n");

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
			int already_len = 0; // 已经接收的长度
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
						printf("recv error. ret:%d, errno:%d\n", ret, errno);
						close(connfd);
						break;
					}
				}
				else if(ret == 0)  // 对方已经断掉
				{
					printf("recv close");
					close(connfd);
					break;
				}
				else
				{
					already_len += ret;
					printf("before process_buffer, already_len:%d, ret:%d\n", already_len, ret);
					if(process_buffer(buffer, already_len))
					{
						printf("error:%d, process_buffer failed\n", __LINE__);
						close(connfd);
						break;
					}
					printf("after process_buffer, already_len:%d\n", already_len);
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
int process_buffer(char *buffer, int &already_len)
{
	int unprocess_buffer_length = already_len; // 未处理的缓冲区长度
	int start = 0;  // 初始的读buffer的起点

	while(1)
	{
		printf("debug, already_len:%d, PacketHeadLength:%d\n", already_len, PacketHeadLength);
		char *start_buffer = buffer+start;
		if(unprocess_buffer_length >= PacketHeadLength) // 长度比包头长
		{
			printf("debug:%d unprocess_buffer_length:%d >= PacketHeadLength:%d\n", __LINE__,unprocess_buffer_length,PacketHeadLength);
			PacketHead header;
			transferBufferToPacketHead(start_buffer, header);
			if(header.uiPacketLen > MaxPacketLength-PacketHeadLength) // 客户端的包，包体太长
			{
				printf("error:%d header.uiPacketLen:%d > MaxPacketContentLength:%d\n", __LINE__, header.uiPacketLen, MaxPacketLength-PacketHeadLength);
				return -1;
			}

			if(unprocess_buffer_length < PacketHeadLength+header.uiPacketLen) // 没有接收完
			{
				printf("debug:%d length:%d < PacketHeadLength+header.uiPacketLen:%d\n", __LINE__, unprocess_buffer_length, PacketHeadLength+header.uiPacketLen);
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
						printf("debug:%d, recv a packet, cmd:%d\n", __LINE__, header.cmd);
						netserver::GetUserNameRequest request;
						if(!request.ParseFromArray(start_buffer+PacketHeadLength,header.uiPacketLen))
						{
							printf("error:%d ParseFromArray, cmd:%d\n", __LINE__, header.cmd);
							return -1;
						}
						printf("success, userid is %d\n",request.userid());
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_len = unprocess_buffer_length;
						break;
					}
					default:
						printf("error:%d recv a packet, cmd:%d\n", __LINE__, header.cmd);
						break;
				}
			}
		}
		else // 包头都不够
		{
			printf("debug:%d unprocess_buffer_length:%d < PacketHeadLength:%d\n", __LINE__,unprocess_buffer_length,PacketHeadLength);
			if(start > 0) // buffer中有完整的包，已经处理过，则要移动buffer
			{
				memmove(buffer, buffer+start, unprocess_buffer_length);
			}
			return 0;
		}
	}
	return 0;
}


