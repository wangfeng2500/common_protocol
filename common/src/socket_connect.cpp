/*
 * Socket_Connect.cpp
 *
 *  Created on: 2016-2-2
 *      Author: wangfeng
 */

#include "socket_connect.h"

Socket_Connect::Socket_Connect()
{
	already_len = 0;
	bzero(buffer,MaxPacketLength);
}

Socket_Connect::~Socket_Connect()
{

}

int Socket_Connect::OnRecv()
{
	API_LOG_DEBUG(LM_ERROR,"enter socket_connect onRecv, ip:%s", _ip.c_str());
	while(1)
	{
		int ret = recv(_sock_fd,buffer+already_len,MaxPacketLength-already_len,0);
		if(ret < 0)
		{
			if(errno == EWOULDBLOCK || errno == EAGAIN) // 没有数据了
			{
				break;
			}
			else if(errno == EINTR)
			{
				continue;
			}
			else
			{
				API_LOG_DEBUG(LM_ERROR,"recv error. ret:%d, errno:%d", ret, errno);
				close(_sock_fd);
				break;
			}
		}
		else if(ret == 0)  // 对方已经断掉
		{
			API_LOG_DEBUG(LM_ERROR,"recv close");
			close(_sock_fd);
			break;
		}
		else
		{
			already_len += ret;
			API_LOG_DEBUG(LM_TRACE,"before process_buffer, already_len:%d, ret:%d", already_len, ret);
			if(msg_input.process_buffer(buffer, already_len) < 0)
			{
				API_LOG_DEBUG(LM_ERROR,"process_buffer failed");
				close(_sock_fd);
				break;
			}
			API_LOG_DEBUG(LM_TRACE,"after process_buffer, already_len:%d", already_len);
		}
	}
	return 0;
}

int Socket_Connect::OnSend()
{
	return 0;
}

int Socket_Connect::OnClose()
{
	return 0;
}

int Socket_Connect::OnError()
{
	return 0;
}
