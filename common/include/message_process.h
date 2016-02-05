/*
 * message_process.h
 *
 *  Created on: 2016年2月2日
 *      Author: fenngwang
 */

#ifndef COMMON_INCLUDE_MESSAGE_PROCESS_H_
#define COMMON_INCLUDE_MESSAGE_PROCESS_H_

#include <stdint.h>
#include "NetDefine.h"
#include "socket_connect.h"
#include <sys/socket.h>

class Socket_Connect;

class Message_Process
{
public:
	Message_Process();
	~Message_Process();

public:
	void setSockConnect(Socket_Connect* sock_connect) { this->sock_connect = sock_connect; };

	/*
	 * return < 0: 出错，调用方需要将socket，并从epoll中去掉
	 * return = 0: 客户端关闭了连接，调用方需要关闭socket，并从epoll中去掉
	 * return > 0: 成功，收到的数据长度（并不是recv_buffer中的剩余长度哦）
	 */
	int recv();

	int send();

private:
	/* input参数：
	      -buffer: 数据缓冲区指针
	 	  -already_recv_len：已经接收的未处理的数据长度
	 * output参数：
		  -already_recv_len：处理后剩余的数据长度,剩余的缓存已经移动到buffer的头部
		  -ret: < 0 出错  = 0 剩余长度不够一个完整包  > 0 为完整包，全部处理完毕
	 */
	int process_buffer(char *buffer, uint32_t &already_recv_len);
private:
	Socket_Connect * sock_connect;
	char recv_buffer[MaxPacketLength]; // 应用层缓接收冲区
	uint32_t recv_buffer_index;        // 接收缓冲区中未处理的数据长度
	uint32_t already_recv_len;         // 已经接收到的长度
	char send_buffer[MaxPacketLength]; // 应用层发送缓冲区
	uint32_t send_buffer_need_len;     // 发送缓冲区中需要发送的数据长度
};



#endif /* COMMON_INCLUDE_MESSAGE_PROCESS_H_ */
