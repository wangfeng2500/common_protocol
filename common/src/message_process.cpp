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
	recv_buffer_index = 0;
	send_buffer_need_len = 0;
	already_recv_len = 0;
	bzero(recv_buffer,MaxPacketLength);
	bzero(send_buffer,MaxPacketLength);
}

Message_Process::~Message_Process()
{

}

int Message_Process::recv()
{
	API_LOG_DEBUG(LM_ERROR,"enter Message_Process onRecv, ip:%s, sequence is %d", sock_connect->getIP().c_str(), sock_connect->getSequence());
	already_recv_len = 0;
	while(1)
	{
		int ret = ::recv(sock_connect->_sock_fd,recv_buffer+recv_buffer_index,MaxPacketLength-recv_buffer_index,0);
		if(ret < 0)
		{
			if(errno == EWOULDBLOCK || errno == EAGAIN) // 没有数据了
			{
				return already_recv_len;
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
			already_recv_len += ret;
			API_LOG_DEBUG(LM_TRACE,"before process_buffer, already_recv_len:%d,recv_buffer_index:%d, ret:%d", already_recv_len,recv_buffer_index, ret);
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

int Message_Process::send()
{
	unsigned int send_data_len = 0;
	API_LOG_DEBUG(LM_ERROR,"begin send, need send %d len data", send_buffer_need_len);
	while(1)
	{
		int ret = ::send(sock_connect->_sock_fd, send_buffer+send_data_len, send_buffer_need_len-send_data_len, MSG_NOSIGNAL);
		if(ret == 0)
		{
			API_LOG_DEBUG(LM_ERROR,"send close");
			return 0;
		}
		else if (ret < 0)
		{
			if((errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) // 不算错误，继续发送
			{
				continue;
			}
			API_LOG_DEBUG(LM_ERROR,"send failed, ret:%d, errno:%d", ret, errno);
			return -1;
		}
		else
		{
			API_LOG_DEBUG(LM_ERROR,"send success, len:%d", ret);
			send_data_len+=ret;
			if(send_data_len >= send_buffer_need_len) // 发完了,清空数据
			{
				send_buffer_need_len = 0;
				bzero(send_buffer,MaxPacketLength);
				return send_data_len;
			}
		}
	}
}

int Message_Process::process_buffer(char *buffer, uint32_t &recv_buffer_index)
{
	uint32_t unprocess_buffer_length = recv_buffer_index; // 未处理的缓冲区长度
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
				API_LOG_DEBUG(LM_ERROR, "header.uiPacketLen:%d > MaxPacketContentLength:%d",  header.uiPacketLen, MaxPacketLength-PacketHeadLength);
				return -1;
			}

			if(unprocess_buffer_length < PacketHeadLength+header.uiPacketLen) // 没有接收完
			{
				//API_LOG_DEBUG(LM_TRACE, "length:%d < PacketHeadLength+header.uiPacketLen:%d",  unprocess_buffer_length, PacketHeadLength+header.uiPacketLen);
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
						netserver::GetUserNameRequest request;
						if(!request.ParseFromArray(start_buffer+PacketHeadLength,header.uiPacketLen))
						{
							API_LOG_DEBUG(LM_ERROR, "ParseFromArray, cmd:%d",  header.cmd);
							return -1;
						}
						API_LOG_DEBUG(LM_TRACE,"success, userid is %d, cmd:%d",request.userid(), header.cmd);
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						recv_buffer_index = unprocess_buffer_length;
						/*
						 * 填充返回数据
						 */
						NetPacket resPacket;
						resPacket.netPacketHead.version = 1;
						resPacket.netPacketHead.cmd = CMD_GetUserName;
						resPacket.netPacketHead.serialNo = 1001;
						resPacket.netPacketHead.result = 0;
						netserver::GetUserNameResponse response;
						response.set_gender(1);
						response.set_name("fenngwang");
						response.set_province("甘肃");
						string strResponse;
						if(!response.SerializeToString(&strResponse))
						{
							resPacket.netPacketHead.result = RetServerFailed;
							resPacket.netPacketHead.uiPacketLen = 0;
							API_LOG_DEBUG(LM_ERROR, "SerializeToString");
						}
						else
						{
							resPacket.netPacketHead.uiPacketLen = strResponse.length();
						}
						if(PacketHeadLength + resPacket.netPacketHead.uiPacketLen <= MaxPacketLength-send_buffer_need_len) // 发送缓冲区未满
						{
							API_LOG_DEBUG(LM_DEBUG, "write %d len data, cmd:%d", PacketHeadLength + resPacket.netPacketHead.uiPacketLen,resPacket.netPacketHead.cmd);
							resPacket.Encode();
							memcpy(send_buffer+send_buffer_need_len, &(resPacket.netPacketHead), PacketHeadLength);
							memcpy(send_buffer+send_buffer_need_len+PacketHeadLength, strResponse.c_str(), strResponse.length());
							send_buffer_need_len+=PacketHeadLength + strResponse.length();
							sock_connect->SetEvent(FD_RECV|FD_SEND|FD_CLOSE|FD_ERROR);
						}
						else
						{
							API_LOG_DEBUG(LM_ERROR,"send buffer is full, cmd:%d",  header.cmd);
						}

						break;
					}
					case CMD_SetUserName:
					{
						/*
						 * 接收数据
						 */
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
						recv_buffer_index = unprocess_buffer_length;
						/*
						 * 填充返回数据
						 */
						NetPacket resPacket;
						resPacket.netPacketHead.version = 1;
						resPacket.netPacketHead.cmd = CMD_SetUserName;
						resPacket.netPacketHead.serialNo = 1001;
						resPacket.netPacketHead.result = 0;
						resPacket.netPacketHead.uiPacketLen = 0;
						if(PacketHeadLength <= MaxPacketLength-send_buffer_need_len) // 发送缓冲区未满
						{
							API_LOG_DEBUG(LM_DEBUG, "write %d len data, cmd:%d", PacketHeadLength,resPacket.netPacketHead.cmd);
							sock_connect->SetEvent(FD_RECV|FD_SEND|FD_CLOSE|FD_ERROR);
							resPacket.Encode();
							memcpy(send_buffer+send_buffer_need_len, &(resPacket.netPacketHead), PacketHeadLength);
							send_buffer_need_len+=PacketHeadLength;
						}
						else
						{
							API_LOG_DEBUG(LM_ERROR,"send buffer is full, cmd:%d",  header.cmd);
						}
						break;
					}
					default:
						API_LOG_DEBUG(LM_ERROR,"recv a undefined packet, cmd:%d",  header.cmd);
						start+=PacketHeadLength+header.uiPacketLen; // 更新起始位置
						unprocess_buffer_length = unprocess_buffer_length - PacketHeadLength-header.uiPacketLen; // 剩余的未读缓冲区长度
						recv_buffer_index = unprocess_buffer_length;
						/*
						 * 填充返回数据
						 */
						NetPacket resPacket;
						resPacket.netPacketHead.version = 1;
						resPacket.netPacketHead.cmd = CMD_GetUserName;
						resPacket.netPacketHead.serialNo = 1001;
						resPacket.netPacketHead.result = -1;
						resPacket.netPacketHead.uiPacketLen = 0;
						if(PacketHeadLength <= MaxPacketLength-send_buffer_need_len) // 发送缓冲区未满
						{
							API_LOG_DEBUG(LM_DEBUG, "write %d len data, cmd:%d", PacketHeadLength,resPacket.netPacketHead.cmd);
							sock_connect->SetEvent(FD_RECV|FD_SEND|FD_CLOSE|FD_ERROR);
							resPacket.Encode();
							memcpy(send_buffer+send_buffer_need_len, &(resPacket.netPacketHead), PacketHeadLength);
							send_buffer_need_len+=PacketHeadLength;
						}
						else
						{
							API_LOG_DEBUG(LM_ERROR,"send buffer is full, cmd:%d",  header.cmd);
						}
						resPacket.Encode();
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
