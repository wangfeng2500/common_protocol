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
#include "log.h"
#include "socket_listen.h"
#include "epoller.h"

using namespace std;
using namespace CGI_LOG;

#define SERVER_PORT 9999

int main()
{
	CGI_Log_Init(__FILE__);

	Socket_Listen socket_listen;
	int ret = socket_listen.Create("127.0.0.1",SERVER_PORT);
	if(ret)
	{
		API_LOG_DEBUG(LM_ERROR, "create listen sockfd failed.");
		exit(-1);
	}

	 CEPoller _epoller;
	 _epoller.Create(65536);
	 socket_listen.AttachEpoller(&_epoller);
	 socket_listen.SetEvent(FD_RECV|FD_CLOSE|FD_ERROR);

	 //启动循环
	 _epoller.LoopForEvent(10);

	 return 0;
}



