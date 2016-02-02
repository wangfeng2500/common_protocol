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

}

Message_Process::~Message_Process()
{

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
						API_LOG_DEBUG(LM_ERROR,"recv a packet, cmd:%d",  header.cmd);
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
