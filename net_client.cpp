/*
 * net_client.cpp
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
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include "NetDefine.h"
#include "log.h"
#include "netserver.pb.h"

using namespace std;
using namespace CGI_LOG;

#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"

int process_buffer(char *buffer, uint32_t &already_recv_len);

int main()
{
	CGI_Log_Init(__FILE__);
	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	int ret = connect(sockfd,(const struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret)
	{
		API_LOG_DEBUG(LM_ERROR, "connect failed, ret:%d, errno:%d",  ret, errno);
		exit(1);
	}

	// 序列化数据
	char buffer[MaxPacketLength] = {0};
	NetPacket obj_1;
	obj_1.netPacketHead.version = 1;
	obj_1.netPacketHead.cmd = CMD_GetUserName;
	obj_1.netPacketHead.serialNo = 1001;
	obj_1.netPacketHead.result = 0;
	netserver::GetUserNameRequest request;
	request.set_userid(1000);
	string strRequest_1;
	if(!request.SerializeToString(&strRequest_1))
	{
		API_LOG_DEBUG(LM_ERROR, "SerializeToString");
		exit(-1);
	}
	obj_1.netPacketHead.uiPacketLen = strRequest_1.length();
	obj_1.Encode();

	NetPacket obj_2;
	obj_2.netPacketHead.version = 2;
	obj_2.netPacketHead.cmd = CMD_GetUserName;
	obj_2.netPacketHead.serialNo = 1002;
	obj_2.netPacketHead.result = 1;
	request.set_userid(1003);
	string strRequest_2;
	if(!request.SerializeToString(&strRequest_2))
	{
		API_LOG_DEBUG(LM_ERROR, "SerializeToString");
		exit(-1);
	}
	obj_2.netPacketHead.uiPacketLen = strRequest_2.length();
	obj_2.Encode();

	NetPacket obj_3;
	obj_3.netPacketHead.version = 3;
	obj_3.netPacketHead.cmd = CMD_SetUserName;
	obj_3.netPacketHead.serialNo = 1003;
	obj_3.netPacketHead.result = 0;

	netserver::SetUserNameRequest setUserName;
	setUserName.set_gender(1);
	setUserName.set_name("fenngwang");
	setUserName.set_province("广东");
	string strUserName;
	if(!setUserName.SerializeToString(&strUserName))
	{
		API_LOG_DEBUG(LM_ERROR, "SerializeToString");
		exit(-1);
	}
	obj_3.netPacketHead.uiPacketLen = strUserName.length();
	obj_3.Encode();

#if 1
	// 一个包，同一个包的头和包体分开发
	send(sockfd,&(obj_1.netPacketHead),PacketHeadLength,0);
	sleep(1);
	send(sockfd,strRequest_1.c_str(),strRequest_1.length(),0);
	sleep(1);
#endif

#if 1
	// 一个包，同一个包的头和包一起发
	memcpy(buffer, &(obj_1.netPacketHead), PacketHeadLength);
	memcpy(buffer+PacketHeadLength, strRequest_1.c_str(),strRequest_1.length());
	send(sockfd,buffer,PacketHeadLength+strRequest_1.length(),0);
	sleep(1);
#endif

#if 1
	// 两个包一起发
	memcpy(buffer, &(obj_1.netPacketHead), PacketHeadLength);
	memcpy(buffer+PacketHeadLength, strRequest_1.c_str(),strRequest_1.length());
	memcpy(buffer+PacketHeadLength+strRequest_1.length(), &(obj_2.netPacketHead), PacketHeadLength);
	memcpy(buffer+2*PacketHeadLength+strRequest_1.length(), strRequest_2.c_str(), strRequest_2.length());
	send(sockfd,buffer,2*PacketHeadLength+strRequest_1.length()+strRequest_2.length(),0);
	sleep(1);
#endif

#if 1
	// 两个包分开发，一个包+第二个的包头，然后第二个的包体
	memcpy(buffer, &(obj_1.netPacketHead), PacketHeadLength);
	memcpy(buffer+PacketHeadLength, strRequest_1.c_str(),strRequest_1.length());
	memcpy(buffer+PacketHeadLength+strRequest_1.length(), &(obj_2.netPacketHead), PacketHeadLength);
	send(sockfd,buffer,2*PacketHeadLength+strRequest_1.length(),0);
	sleep(1);
	memcpy(buffer, strRequest_2.c_str(), strRequest_2.length());
	send(sockfd,buffer,strRequest_2.length(),0);
	sleep(1);
#endif

#if 1
	// 两个包分开发，一个包+第二个的包头-1，然后第二个的包头最后1+包体
	memcpy(buffer, &(obj_1.netPacketHead), PacketHeadLength);
	memcpy(buffer+PacketHeadLength, strRequest_1.c_str(),strRequest_1.length());
	memcpy(buffer+PacketHeadLength+strRequest_1.length(), &(obj_2.netPacketHead), PacketHeadLength-1);
	send(sockfd,buffer,2*PacketHeadLength+strRequest_1.length()-1,0);
	sleep(1);
	memcpy(buffer, (char *)&(obj_2.netPacketHead)+PacketHeadLength-1, 1);
	memcpy(buffer+1, strRequest_2.c_str(), strRequest_2.length());
	send(sockfd,buffer,1+strRequest_2.length(),0);
	sleep(1);
#endif

	// 再发另外一个命令的包
	memcpy(buffer, &(obj_3.netPacketHead), PacketHeadLength);
	memcpy(buffer+PacketHeadLength, strUserName.c_str(),strUserName.length());
	send(sockfd,buffer,PacketHeadLength+strUserName.length(),0);

	char recv_buffer[MaxPacketLength];
	uint32_t recv_buffer_index = 0;
	while(1)
	{
		int ret = ::recv(sockfd,recv_buffer+recv_buffer_index,MaxPacketLength-recv_buffer_index,0);
		if(ret < 0)
		{
			if(errno == EWOULDBLOCK || errno == EAGAIN) // 没有数据了
			{
				return 1;
			}
			else if(errno == EINTR)
			{
				continue;
			}
			else
			{
				API_LOG_DEBUG(LM_ERROR,"recv error. ret:%d, errno:%d", ret, errno);
				return -1;
			}
		}
		else if(ret == 0)  // 对方已经断掉
		{
			API_LOG_DEBUG(LM_ERROR,"recv close");
			return 0;
		}
		else
		{
			recv_buffer_index += ret;
			API_LOG_DEBUG(LM_TRACE,"before process_buffer, recv_buffer_index:%d, ret:%d", recv_buffer_index, ret);
			if(process_buffer(recv_buffer, recv_buffer_index) < 0)
			{
				API_LOG_DEBUG(LM_ERROR,"process_buffer failed");
				return -1;
			}
			API_LOG_DEBUG(LM_TRACE,"after process_buffer, recv_buffer_index:%d", recv_buffer_index);
		}
	}

	return 0;
}

int process_buffer(char *buffer, uint32_t &already_recv_len)
{
	uint32_t unprocess_buffer_length = already_recv_len; // 未处理的缓冲区长度
	int start = 0;  // 初始的读buffer的起点

	while(1)
	{
		API_LOG_DEBUG(LM_TRACE, "unprocess_buffer_length:%d, PacketHeadLength:%d", unprocess_buffer_length, PacketHeadLength);
		char *start_buffer = buffer+start;
		if(unprocess_buffer_length >= PacketHeadLength) // 长度比包头长
		{
			PacketHead header;
			transferBufferToPacketHead(start_buffer, header);
			if(header.uiPacketLen > MaxPacketLength-PacketHeadLength) // 客户端的包，包体太长
			{
				API_LOG_DEBUG(LM_TRACE, "header.uiPacketLen:%d > MaxPacketContentLength:%d",  header.uiPacketLen, MaxPacketLength-PacketHeadLength);
				return -1;
			}

			if(unprocess_buffer_length < PacketHeadLength+header.uiPacketLen) // 没有接收完
			{
				API_LOG_DEBUG(LM_TRACE, "length:%d < PacketHeadLength+header.uiPacketLen:%d",  unprocess_buffer_length, PacketHeadLength+header.uiPacketLen);
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
						/*
						 * 接收数据
						 */
						API_LOG_DEBUG(LM_TRACE,"recv a packet, cmd:%d",  header.cmd);
						netserver::GetUserNameResponse request;
						if(!request.ParseFromArray(start_buffer+PacketHeadLength,header.uiPacketLen))
						{
							API_LOG_DEBUG(LM_ERROR, "ParseFromArray, cmd:%d",  header.cmd);
							return -1;
						}
						API_LOG_DEBUG(LM_TRACE,"success, name is %s, province is %s",request.name().c_str(), request.province().c_str());
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_recv_len = unprocess_buffer_length;
						break;
					}
					case CMD_SetUserName:
					{
						/*
						 * 接收数据
						 */
						API_LOG_DEBUG(LM_TRACE," success, recv a packet, cmd:%d, result:%d",  header.cmd), header.result;
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_recv_len = unprocess_buffer_length;
						break;
					}
					default:
						API_LOG_DEBUG(LM_ERROR,"success, recv a undefined packet, cmd:%d",  header.cmd);
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_recv_len = unprocess_buffer_length;
						break;
				}
			}
		}
		else // 包头都不够
		{
			API_LOG_DEBUG(LM_TRACE, "unprocess_buffer_length:%d < PacketHeadLength:%d", unprocess_buffer_length,PacketHeadLength);
			if(start > 0) // buffer中有完整的包，已经处理过，则要移动buffer
			{
				memmove(buffer, buffer+start, unprocess_buffer_length);
			}
			return 0;
		}
	}
	return 0;
}
