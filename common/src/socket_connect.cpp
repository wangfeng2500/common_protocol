/*
 * Socket_Connect.cpp
 *
 *  Created on: 2016-2-2
 *      Author: wangfeng
 */

#include "socket_connect.h"

Socket_Connect::Socket_Connect()
{
	_sock_listen = NULL;
	msg_input = new Message_Process();
	msg_input->setSockConnect(this);
	seqence++;
}

Socket_Connect::~Socket_Connect()
{
	delete msg_input;
}

int Socket_Connect::OnRecv()
{
	int ret = msg_input->recv();
	if(ret == 0)  // 对方关闭了连接
	{
		API_LOG_DEBUG(LM_DEBUG,"ip:%s close conection", _ip.c_str());
		delete this;
	}
	else if(ret < 0) // 出错
	{
		API_LOG_DEBUG(LM_DEBUG,"ip:%s recv error", _ip.c_str());
		delete this;
	}
	else
	{
		//do nothing
	}
	return 0;
}

int Socket_Connect::OnSend()
{
	int ret = msg_input->send();
	if(ret == 0)  // 对方关闭了连接
	{
		API_LOG_DEBUG(LM_DEBUG,"ip:%s close conection", _ip.c_str());
		delete this;
	}
	else if(ret < 0) // 出错
	{
		API_LOG_DEBUG(LM_DEBUG,"ip:%s recv error", _ip.c_str());
		delete this;
	}
	else
	{
		API_LOG_DEBUG(LM_DEBUG,"Socket_Connect, success send %d len data", ret);
		this->SetEvent(FD_RECV|FD_CLOSE|FD_ERROR); //发送完成后去掉send的事件监听
	}
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
