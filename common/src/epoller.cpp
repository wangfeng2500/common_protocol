#include "epoller.h"
//#include "idle_conn_pool.h"
//extern CIdleConnPool g_idle_conn_pool;

CObjectMap* CObjectMap::_ins = NULL;
CObjectMap* CObjectMap::Instance()
{
	if ( !_ins )
		_ins = new CObjectMap();
	return _ins;
}

CObjectMap::CObjectMap()
{
	hash_table_size = 0;
	hash_table = NULL;
}

CObjectMap::~CObjectMap()
{
	if ( hash_table )
	{
		DropMap();
	}
}

int CObjectMap::CreateMap(int map_size)
{
	hash_table = new struct OBJECTMAP[map_size];
	if ( !hash_table ) return -1;
	
	for(int i=0;i<map_size;i++)
	{
		hash_table[i].id = 0;
		hash_table[i].objaddr = NULL;
		INIT_LIST_HEAD(&hash_table[i].list_item);
	}
	hash_table_size = map_size;
	
	return 0;
}

int CObjectMap::AddMapObj(int id,void* obj)
{
	int idx = id%hash_table_size;
	
	struct OBJECTMAP* item;
	item = new struct OBJECTMAP();
	if ( !item )
		return -1;
	
	item->id = id;
	item->objaddr = obj;
	list_add_tail(&item->list_item,&hash_table[idx].list_item);	
	
	return 0;
}

int CObjectMap::DelMapObj(int id)
{
	int idx = id%hash_table_size;
	
	struct list_head* _head;
	struct list_head* _tmp1;
	struct list_head* _tmp2;
	struct OBJECTMAP*  item;
	
	_head = &hash_table[idx].list_item;
	list_for_each_safe(_tmp1,_tmp2,_head)
	{
		item = list_entry(_tmp1,struct OBJECTMAP,list_item);
		if ( item->id == id )
		{
			list_del(_tmp1);
			delete item;
			return 0;
		}
	}
	
	return -1;
}

void* CObjectMap::GetMapObj(int id)
{
	int idx = id%hash_table_size;
	
	struct list_head* _head;
	struct list_head* _tmp;
	struct OBJECTMAP*  item;
	
	_head = &hash_table[idx].list_item;
	list_for_each(_tmp,_head)
	{
		item = list_entry(_tmp,struct OBJECTMAP,list_item);
		if ( item->id == id )
		{
			return item->objaddr;
		}
	}
	
	return NULL;
}

int CObjectMap::DropMap()
{
	struct list_head* _head;
	struct list_head* _tmp1;
	struct list_head* _tmp2;
	struct OBJECTMAP*  item;
	
	for( int i=0;i<hash_table_size;i++ )
	{
		_head = &hash_table[i].list_item;
		list_for_each_safe(_tmp1,_tmp2,_head)
		{
			item = list_entry(_tmp1,struct OBJECTMAP,list_item);
			list_del(_tmp1);
			delete item;
		}
	}
	
	delete[] hash_table;
	hash_table = NULL;
	hash_table_size = 0;
	
	return 0;
}



CEPoller::CEPoller()
{
	_epoll_fd = -1;
}

CEPoller::~CEPoller()
{
	if(_epoll_fd != -1)
	{
		close(_epoll_fd);
	}
}

int CEPoller::Create(int maxfd)
{
	_epoll_fd = epoll_create(maxfd);
	if ( _epoll_fd <= 0 )
	{
		memset(_err_msg,0,NET_ERRMSG_SIZE);
		snprintf(_err_msg,NET_ERRMSG_SIZE,"epoller create size:%d error:%s\n",maxfd,strerror(errno));
		return -1;
	}
	
	if ( _obj_map.CreateMap(maxfd) < 0 )
	{
		memset(_err_msg,0,NET_ERRMSG_SIZE);
		snprintf(_err_msg,NET_ERRMSG_SIZE,"epoller create obj-map:%d error\n",maxfd);
		return -1;
	}
	
	return 0;
}

void CEPoller::AttachSocket(CEpollSocket* sock)
{
	int fd = sock->GetSockHandle();
	if ( fd > 0 )
		_obj_map.AddMapObj(fd,(void*)sock);
	
	return ;
}

void CEPoller::DetachSocket(CEpollSocket* sock)
{
	int fd = sock->GetSockHandle();
	if ( fd > 0 )
	{
		DelEpollIO(fd);
		_obj_map.DelMapObj(fd);
	}
	
	return ;
}

int CEPoller::LoopForEvent(int timeout)
{
	int fd;
	int nfds;
	CEpollSocket*  sock;
	unsigned ev;
	struct timeval prev_tm;
	struct timeval next_tm;
	long	use_time_usec;
	gettimeofday(&prev_tm,NULL);
	
	for(;;)
	{
		nfds = epoll_wait(_epoll_fd, _events, EPOLL_FD_MAX, timeout);
		
		if (nfds < 0)
		{
			if ( errno == EINTR )
				continue;
			
			memset(_err_msg,0,NET_ERRMSG_SIZE);
			snprintf(_err_msg,NET_ERRMSG_SIZE,"epoll-wait rtn:%d error:%s\n",nfds,strerror(errno));
			return -1;
		}

		for( int i=0;i<nfds;i++ )
		{
			fd = _events[i].data.fd;
			sock = (CEpollSocket*)_obj_map.GetMapObj(fd);
			if ( sock == NULL )
			{
        //        g_idle_conn_pool.RemoveFd(fd);
				API_LOG_DEBUG(LM_DEBUG, "111111111111111111111111111111111111111111");
				DelEpollIO(fd);	close(fd);
				continue;
			}
			
			ev = _events[i].events;
			if ( ev&EPOLLIN )
				sock->OnRecv();							
			else if ( ev&EPOLLOUT )
				sock->OnSend();
			else if ( ev&EPOLLHUP )
				sock->OnClose();
			else if ( ev&EPOLLERR )
				sock->OnError();
			else
				sock->OnError();
		}
		
#if 0
		gettimeofday(&next_tm,NULL);
		use_time_usec = (next_tm.tv_sec - prev_tm.tv_sec)*1000000 +
						(next_tm.tv_usec - prev_tm.tv_usec);

		if ( use_time_usec > (1000))
		{
			CTimer::Ins()->CheckTimeOut(next_tm);
            CTimer::InsSpecial()->CheckTimeOut(next_tm);
			CTimer::Ins2()->CheckTimeOut(next_tm,2);
			prev_tm = next_tm;
		}
#endif
	}
}

char * CEPoller::GetErrMsg()
{
	return _err_msg;
}

int CEPoller::SetEpollIO(int fd,unsigned flag)
{
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = flag|EPOLLHUP|EPOLLERR;
	
	if ( epoll_ctl(_epoll_fd, EPOLL_CTL_MOD , fd, &ev) < 0 )	
	{
		if ( epoll_ctl(_epoll_fd, EPOLL_CTL_ADD , fd, &ev) < 0 )
		{
			memset(_err_msg,0,NET_ERRMSG_SIZE);
			snprintf(_err_msg,NET_ERRMSG_SIZE,"epoll_ctl fd:%d err:%s\n",fd,strerror(errno));
			return -1;
		}
	}
	
	return 0;
}

int CEPoller::AddEpollIO(int fd,unsigned flag)
{
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = flag;
	
	if ( epoll_ctl(_epoll_fd, EPOLL_CTL_ADD , fd, &ev) < 0 )
		return -1;
	
	return 0;
}

int CEPoller::ModEpollIO(int fd,unsigned flag)
{
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = flag;
	
	if ( epoll_ctl(_epoll_fd, EPOLL_CTL_MOD , fd, &ev) < 0 )	
	{
		return -1;
	}
	
	return 0;
}

int CEPoller::DelEpollIO(int fd)
{
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = 0;
	if ( epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &ev) < 0 )
		return -1;
	
	return 0;
}

CEpollSocket::CEpollSocket()
{
	_event_flag = false;
	_sock_fd = -1;
	_epoller = NULL;
	m_iLSID = 0;
	m_mod_ret = 0;
}

CEpollSocket::~CEpollSocket()
{
	DropSocket();
}

int CEpollSocket::GetSockHandle()
{
	return _sock_fd;
}

int CEpollSocket::SetSockHandle(int fd)
{
	_sock_fd = fd;
	_event_flag = false;
	
	return 0;
}

int CEpollSocket::AttachEpoller(CEPoller* epoller)
{
	_epoller = epoller;
	_event_flag = false;
	if ( _epoller ) _epoller->AttachSocket(this);
	
	return 0;
}

int CEpollSocket::DetachEpoller()
{
	if ( _epoller ) _epoller->DetachSocket(this);
	_epoller = NULL;
	
	return 0;
}

int CEpollSocket::SetEvent(unsigned event)
{
	if ( !_epoller ) return -1;
	
	if ( _epoller->ModEpollIO(_sock_fd,event) < 0 )
		return _epoller->AddEpollIO(_sock_fd,event);
	
	return 0;
}

int CEpollSocket::DropSocket()
{
	if ( _sock_fd )
	{
		DetachEpoller();
		close(_sock_fd);
	}
	_sock_fd = -1;
	_epoller = NULL;
	
	return 0;
}
