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

class Message_Process
{
public:
	Message_Process();
	~Message_Process();

public:
	/* input参数：
	      -buffer: 数据缓冲区指针
	 	  -already_recv_len：已经接收的未处理的数据长度
	 * output参数：
		  -already_recv_len：处理后剩余的数据长度
		  -ret: < 0 出错  = 0 剩余长度不够一个完整包  > 0 为完整包，全部处理完毕
	 */
	int process_buffer(char *buffer, uint32_t &already_recv_len);
};



#endif /* COMMON_INCLUDE_MESSAGE_PROCESS_H_ */
