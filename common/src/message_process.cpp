/*
 * message_process.cpp
 *
 *  Created on: 2016年2月2日
 *      Author: fenngwang
 */
#include "message_process.h"
#include "log.h"
#include "netserver.pb.h"
#include <iostream>
#include <string>

using namespace std;
using namespace CGI_LOG;

Message_Process::Message_Process()
{
	sock_connect = NULL;
	already_len = 0;
	bzero(recv_buffer,MaxPacketLength);
	bzero(send_buffer,MaxPacketLength);
}

Message_Process::~Message_Process()
{

}

int Message_Process::recv()
{
	API_LOG_DEBUG(LM_ERROR,"enter Message_Process onRecv, ip:%s", sock_connect->getIP().c_str());
	while(1)
	{
		int ret = ::recv(sock_connect->_sock_fd,recv_buffer+already_len,MaxPacketLength-already_len,0);
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
			already_len += ret;
			API_LOG_DEBUG(LM_TRACE,"before process_buffer, already_len:%d, ret:%d", already_len, ret);
			if(process_buffer(recv_buffer, already_len) < 0)
			{
				API_LOG_DEBUG(LM_ERROR,"process_buffer failed");
				return -1;
			}
			API_LOG_DEBUG(LM_TRACE,"after process_buffer, already_len:%d", already_len);
		}
	}
	return 0;
}

int Message_Process::send()
{
	return 0;
}

int Message_Process::process_buffer(char *buffer, uint32_t &already_recv_len)
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
						API_LOG_DEBUG(LM_TRACE,"recv a packet, cmd:%d",  header.cmd);
						netserver::GetUserNameRequest request;
						if(!request.ParseFromArray(start_buffer+PacketHeadLength,header.uiPacketLen))
						{
							API_LOG_DEBUG(LM_ERROR, "ParseFromArray, cmd:%d",  header.cmd);
							return -1;
						}
						API_LOG_DEBUG(LM_TRACE,"success, userid is %d",request.userid());
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_recv_len = unprocess_buffer_length;
						break;
					}
					case CMD_SetUserName:
					{
						API_LOG_DEBUG(LM_TRACE," recv a packet, cmd:%d",  header.cmd);
						netserver::SetUserNameRequest request;
						if(!request.ParseFromArray(start_buffer+PacketHeadLength,header.uiPacketLen))
						{
							API_LOG_DEBUG(LM_ERROR, "ParseFromArray, cmd:%d",  header.cmd);
							return -1;
						}
						API_LOG_DEBUG(LM_TRACE,"success, gender is %d, name is %s, province is %s",request.gender(), request.name().c_str(), request.province().c_str());
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						already_recv_len = unprocess_buffer_length;
						break;
					}
					default:
						API_LOG_DEBUG(LM_ERROR,"recv a undefined packet, cmd:%d",  header.cmd);
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
