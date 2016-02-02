#ifndef NET_DEFINE_H
#define NET_DEFINE_H

#include <stdint.h>
#include <string.h>
#include "log.h"

using namespace CGI_LOG;

#define MaxPacketLength 65536

#pragma pack(push,1)   //将原对齐方式压栈,采用新的1字节对齐方式

/* 封包类型枚举[此处根据需求列举] */
typedef enum
{
	CMD_GetUserName = 1,
	CMD_SetUserName = 2
} PacketCmd;

typedef struct stPacketHead
{
	uint32_t version;         // 版本
	PacketCmd cmd;            // 命令字
	uint32_t uiPacketLen;     // 包体长度
	uint32_t result;          // 回包专用字段
}PacketHead;

/* 封包对象[包头&包体] */
typedef struct tagNetPacket
{
	PacketHead netPacketHead;//包头
	char * packetBody;//包体
} NetPacket;


#define transferBufferToPacketHead(buffer, header)\
{\
	memcpy(&(header.version), buffer, sizeof(uint32_t));\
	memcpy(&(header.cmd), buffer+sizeof(uint32_t), sizeof(PacketCmd));\
	memcpy(&(header.uiPacketLen), buffer+sizeof(uint32_t)+sizeof(PacketCmd), sizeof(uint32_t));\
	memcpy(&(header.result), buffer+2*sizeof(uint32_t)+sizeof(PacketCmd), sizeof(uint32_t));\
	API_LOG_DEBUG(LM_TRACE,"version:%d, cmd:%d, uiPacketLen:%d, result:%d\n",header.version, header.cmd, header.uiPacketLen,header.result);\
}
//void transferPacketHeadToBuffer(const PacketHead &header, char *buffer)
//{
//	memcpy(&(obj_1.netPacketHead.version), &version, sizeof(uint32_t));
//	memcpy(&(obj_1.netPacketHead.cmd), &cmd, sizeof(PacketCmd));
//	memcpy(&(obj_1.netPacketHead.uiPacketLen), &uiPacketLen, sizeof(uint32_t));
//	memcpy(&(obj_1.netPacketHead.result), &result, sizeof(uint32_t));
//}

#pragma pack(pop)

const uint32_t  PacketHeadLength = sizeof(PacketHead);

#endif
