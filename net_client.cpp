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
#include "netserver.pb.h"

using namespace std;

#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"

int main()
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	int ret = connect(sockfd,(const struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret)
	{
		printf("error:%d, connect failed, ret:%d, errno:%d\n", __LINE__, ret, errno);
		exit(1);
	}

	// 序列化数据
	char buffer[MaxPacketLength] = {0};
	NetPacket obj_1;
	obj_1.netPacketHead.version = 179;
	obj_1.netPacketHead.cmd = CMD_GetUserName;
	obj_1.netPacketHead.result = 0;
	netserver::GetUserNameRequest request;
	request.set_userid(1000);
	string strRequest_1;
	if(!request.SerializeToString(&strRequest_1))
	{
		printf("error:%d SerializeToString\n", __LINE__);
		exit(-1);
	}
	obj_1.netPacketHead.uiPacketLen = strRequest_1.length();

	NetPacket obj_2;
	obj_2.netPacketHead.version = 180;
	obj_2.netPacketHead.cmd = CMD_GetUserName;
	obj_2.netPacketHead.result = 1333;
	request.set_userid(1003);
	string strRequest_2;
	if(!request.SerializeToString(&strRequest_2))
	{
		printf("error:%d SerializeToString\n", __LINE__);
		exit(-1);
	}
	obj_2.netPacketHead.uiPacketLen = strRequest_2.length();

	NetPacket obj_3;
	obj_3.netPacketHead.cmd = CMD_SetUserName;
	obj_3.netPacketHead.version = 10;
	obj_3.netPacketHead.result = 0;
	netserver::SetUserNameRequest setUserName;
	setUserName.set_gender(1);
	setUserName.set_name("fenngwang");
	setUserName.set_province("广东");
	string strUserName;
	if(!setUserName.SerializeToString(&strUserName))
	{
		printf("error:%d SerializeToString\n", __LINE__);
		exit(-1);
	}
	obj_3.netPacketHead.uiPacketLen = strUserName.length();

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

	return 0;
}
