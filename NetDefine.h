#include <stdint.h>
#include <string.h>

#define MaxPacketLength 65536

#pragma pack(push,1)   //将原对齐方式压栈,采用新的1字节对齐方式

/* 封包类型枚举[此处根据需求列举] */
typedef enum
{
	CMD_GetUserName = 1,
	CMD_GetUserAddress = 2
} PacketCmd;

typedef struct stPacketHead
{
	uint32_t version;         // 版本
	PacketCmd cmd;       // 命令字
	uint32_t uiPacketLen; // 包体长度
}PacketHead;

/* 封包对象[包头&包体] */
typedef struct tagNetPacket
{
	PacketHead netPacketHead;//包头
	char * packetBody;//包体
} NetPacket;

//#define transferBufferToPacketHead(buffer, header)  \
//	memcpy(&(header.version), buffer, sizeof(uint32_t)); \
//	memcpy(&(header.cmd), buffer+sizeof(uint32_t), sizeof(PacketCmd));\
//	memcpy(&(header.uiPacketLen), buffer+sizeof(uint32_t)+sizeof(PacketCmd), sizeof(uint32_t));

void transferBufferToPacketHead(char * buffer, PacketHead &header)
{
	memcpy(&(header.version), buffer, sizeof(uint32_t));
	memcpy(&(header.cmd), buffer+sizeof(uint32_t), sizeof(PacketCmd));
	memcpy(&(header.uiPacketLen), buffer+sizeof(uint32_t)+sizeof(PacketCmd), sizeof(uint32_t));
	printf("version:%d, cmd:%d, uiPacketLen:%d\n",header.version, header.cmd, header.uiPacketLen);
}

#pragma pack(pop)

const int PacketHeadLength = sizeof(PacketHead);
