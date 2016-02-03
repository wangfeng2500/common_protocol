/*
 * socket_connect.h
 *
 *  Created on: 2016-2-2
 *      Author: wangfeng
 */

#ifndef SOCKET_CONNECT_H_
#define SOCKET_CONNECT_H_

#include "epoller.h"
#include "socket_listen.h"
#include "message_process.h"

class Socket_Listen;

class Socket_Connect:public CEpollSocket
{
public:
	Socket_Connect();
	~Socket_Connect();

	virtual int OnRecv();
	virtual int OnSend();
	virtual int OnClose();
	virtual int OnError();

	void SetSockListen(Socket_Listen* sock_listen) { _sock_listen = sock_listen; };

	void setClientIP(const char * strIP){_ip = strIP;}

private:
	Socket_Listen *_sock_listen;
	string _ip;
	char buffer[MaxPacketLength]; //应用层缓冲区
	uint32_t already_len;         // 已经接收的长度
	Message_Process msg_input;    // 数据解析类
};




#endif /* SOCKET_CONNECT_H_ */
