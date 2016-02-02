#ifndef NET_DEFINE_H
#define NET_DEFINE_H

#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "log.h"

using namespace CGI_LOG;

#define MaxPacketLength 65536

/* 命令字 */
typedef enum
{
	CMD_NULL = 0,
	CMD_GetUserName = 1,
	CMD_SetUserName = 2
} PacketCmd;

typedef enum
{
	RetServerSucc = 0,	    // 0 - [正常数据, 处理成功]
	RetServerFailed = 1,	// 1 - [正常数据, 处理失败]
	QzoneServerBusy = 2,	// 2 - [正常数据, 服务器忙, 可重试]
}RetServerResponse;

/*
 * protocol head
 -----------------------------------------------------------------------------------------------------------
| 版本(4 bytes) | 命令字(4 bytes) | 序列号(4 bytes) | server回应标识(4 byte) | 协议总长度(4 bytes) | 协议体 |
 -----------------------------------------------------------------------------------------------------------
 */
#pragma pack(push,1)
typedef struct stPacketHead
{
	uint32_t version;         // 版本
	uint32_t cmd;            // 命令字
	uint32_t serialNo;        // 序列号
	uint32_t result;          // 回包专用字段
	uint32_t uiPacketLen;     // 包体长度

	stPacketHead()
	{
		version = 0;
		cmd = CMD_NULL;
		serialNo = 0;
		result = 0;
		uiPacketLen = 0;
	}

	void Encode()
	{
		version = htonl(version);
		cmd = htonl(cmd);
		serialNo = htonl(serialNo);
		result = htonl(result);
		uiPacketLen = htonl(uiPacketLen);
	}

	void Decode()
	{
		version = ntohl(version);
		cmd = ntohl(cmd);
		serialNo = ntohl(serialNo);
		result = ntohl(result);
		uiPacketLen = ntohl(uiPacketLen);
	}

}PacketHead;

/* 封包对象[包头&包体] */
typedef struct
{
	PacketHead netPacketHead;//包头
	char * packetBody;//包体
	void Encode()
	{
		netPacketHead.Encode();
	}
} NetPacket;
#pragma pack(pop)

const uint32_t  PacketHeadLength = sizeof(PacketHead);

#define transferBufferToPacketHead(buffer, header)\
{\
	memcpy(&(header.version), buffer, sizeof(uint32_t));\
	memcpy(&(header.cmd), buffer+sizeof(uint32_t), sizeof(uint32_t));\
	memcpy(&(header.serialNo), buffer+2*sizeof(uint32_t), sizeof(uint32_t));\
	memcpy(&(header.result), buffer+3*sizeof(uint32_t), sizeof(uint32_t));\
	memcpy(&(header.uiPacketLen), buffer+4*sizeof(uint32_t), sizeof(uint32_t));\
	header.Encode();\
	API_LOG_DEBUG(LM_TRACE,"version:%d, cmd:%d, serialNo:%d, uiPacketLen:%d, result:%d\n",header.version, header.cmd, header.serialNo, header.uiPacketLen,header.result);\
}

#endif
