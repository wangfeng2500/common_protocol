#ifndef _EPOLLER_H_
#define _EPOLLER_H_
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <assert.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <sys/time.h>
#include "list.h"
#include "log.h"

using namespace std;
using namespace CGI_LOG;

#ifndef NET_ERRMSG_SIZE
#define NET_ERRMSG_SIZE	512
#endif

class CEPoller;
class CEpollSocket;
struct OBJECTMAP
{
	int 	id;
	void* 	objaddr;
	struct list_head list_item;
};

class CObjectMap
{
public:
	CObjectMap();
	~CObjectMap();
	
	int CreateMap(int map_size);
	int AddMapObj(int id,void* obj);
	int DelMapObj(int id);
	void* GetMapObj(int id);
	int DropMap();
	static CObjectMap* Instance();
private:
	static CObjectMap* _ins;
	struct OBJECTMAP* hash_table;
	int hash_table_size;
};


#define EPOLL_FD_MAX		10240
class CEPoller
{
public:
	CEPoller();
	~CEPoller();
	
	int Create(int maxfd);
	int AddEpollIO(int fd,unsigned flag);
	int ModEpollIO(int fd,unsigned flag);
	int	SetEpollIO(int fd,unsigned flag);
	int DelEpollIO(int fd);
	void AttachSocket(CEpollSocket* sock);
	void DetachSocket(CEpollSocket* sock);
	int  LoopForEvent(int timeout);
	char * GetErrMsg();
protected:
	char			_err_msg[NET_ERRMSG_SIZE];
	int 			_epoll_fd;				//epoll的句柄
	epoll_event		_events[EPOLL_FD_MAX];	//epoll_wait的返回的事件
	int _maxfd;
	
	CObjectMap		_obj_map;
};

#define FD_RECV		EPOLLIN
#define FD_SEND		EPOLLOUT
#define FD_CLOSE	EPOLLHUP
#define FD_ERROR	EPOLLERR
class CEpollSocket
{
public:
	CEpollSocket();
	virtual ~CEpollSocket();
	
	virtual int OnRecv(){return 0;};
	virtual int OnSend(){return 0;};
	virtual int OnClose(){return 0;};
	virtual int OnError(){return 0;};
	
	int GetSockHandle();
	int SetSockHandle(int fd);
	int AttachEpoller(CEPoller* epoller);
	int DetachEpoller();
	
	int SetEvent(unsigned event);
	int DropSocket();
	CEPoller*			_epoller;	//关联的epoller
	int					_sock_fd;	//数据处理的句柄
protected:
	bool				_event_flag;
	int m_iLSID;
	int m_mod_ret;
    
};


#endif
