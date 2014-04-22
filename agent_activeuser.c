
/**活动用户链表定义及操作**/

#include "agent_activeuser.h"



#define TIMER 5     //轮询次数为5次，每次一分钟

#define SLEEPTIME 60  //60s轮询一次

//链表表头
ActiveUser *ActiveList;

//链表操作锁
pthread_mutex_t active_list_lock = PTHREAD_MUTEX_INITIALIZER;;


//初始化链表操作
void initActiveList()
{
	//新建头结点并初始化
	ActiveList = (ActiveUser *)malloc(sizeof(ActiveUser));
	memset(ActiveList,0,sizeof(ActiveUser));
}

//增加节点
int addActiveUser(char *username,struct sockaddr_in* ptrUaddr,char *account,struct sockaddr_in* ptrSaddr)
{
	if((username == NULL) || (ptrUaddr == NULL) || (account == NULL) || (ptrSaddr == NULL))
	{
			debug("add active user error,pUsername=%lu,pUaddr=%lu,pAccount=%lu,pSaddr=%lu",(unsigned long)username,(unsigned long)ptrUaddr,(unsigned long)account,(unsigned long)ptrSaddr);
			return -1;
	}

	//先轮训查找节点是否已经存在，若存在则查看地址是否相同，相同则返回，不同删除原来地址，如不存在则直接新建
	pthread_mutex_lock(&active_list_lock);
	ActiveUser *au = ActiveList->next, *preNode = ActiveList;
	while((au != NULL) && (au->timer != 0))
	{
		if(!memcmp(au->username,username,strlen(username)))  //用户名相同
		{
			if((ptrUaddr->sin_addr.s_addr == au->useraddr.sin_addr.s_addr) && (ptrUaddr->sin_port == au->useraddr.sin_port) && (ptrSaddr->sin_addr.s_addr == au->serveraddr.sin_addr.s_addr) && (ptrSaddr->sin_port == au->serveraddr.sin_port)) //双方地址也相同,即不用加入了
			{
				//debug("duplicate node in active user list");
				pthread_mutex_unlock(&active_list_lock);
				au->timer = TIMER;
				return 0;
			}
			//双方地址不相同，删除原节点
			else
				au->timer = 0;   //设置timer为0,到时自动删除
		}
		preNode = au;
		au = preNode->next;
	}

	pthread_mutex_unlock(&active_list_lock);
	//新建节点并初始化
	au = (ActiveUser *)malloc(sizeof(ActiveUser));
	memset(au,0,sizeof(ActiveUser));
	//新节点赋各项值
	memcpy(au->username,username,strlen(username));
	memcpy(&(au->useraddr),ptrUaddr,sizeof(struct sockaddr_in));
	memcpy(au->account,account,strlen(account));
	memcpy(&(au->serveraddr),ptrSaddr,sizeof(struct sockaddr_in));
	au->timer = TIMER;

	//需要加锁，所有与列表相关的操作均放在锁内，以防出问题
	pthread_mutex_lock(&active_list_lock);
	//将新节点作为第一个有效节点
	au->next = ActiveList->next;
	ActiveList->next = au;
	//解锁
	pthread_mutex_unlock(&active_list_lock);

	return 0;
}

//根据用户名找到某个节点
ActiveUser *getActiveUserByName(char *username,struct sockaddr_in *useraddr,struct sockaddr_in *serveraddr,enum ToAddrLabel type)
{
	if((username == NULL) || ((useraddr == NULL) && (serveraddr == NULL)) || (type == TAL_NONE))
	{
			debug("get active user by name error,pUsername=%lu,pUaddr=%lu,pSaddr=%lu,type=%1u",(unsigned long)username,(unsigned long)useraddr,(unsigned long)serveraddr,type);
			return NULL;
	}

	//需要加锁,防止搜索的时候有节点被删除
	pthread_mutex_lock(&active_list_lock);
	ActiveUser *au = ActiveList->next;
	//debug("username : %s\n",username);
	//debug("au is : %d\n",(int)au);
	//循环搜索匹配节点
	while((au != NULL) && (au->timer != 0))
	{
		//debug("au->username is : %s\n",au->username);
		if(!memcmp(au->username,username,strlen(username)))  //找到所需节点
		{
			//给套接字赋值
			if(type == TAL_ToServer)
			{
				if(serveraddr == NULL)
				{
					pthread_mutex_unlock(&active_list_lock);
					debug("when send to server,pServer is NULL");
					return NULL;
				}
				memcpy(serveraddr,&(au->serveraddr),sizeof(struct sockaddr_in));
				au->timer = TIMER;   //加满节点时间，放于锁内可防止节点出锁即被删除(调用此方法就意味着一定会发送信息给对方)
			}
			else if(type == TAL_ToUser)
			{
				if(useraddr == NULL)
				{
					debug("when send to user,pUser is NULL");
					pthread_mutex_unlock(&active_list_lock);
					return NULL;
				}
				memcpy(useraddr,&(au->useraddr),sizeof(struct sockaddr_in));  //服务器传送信息并不一定表明客户端依然在连接，所以不修改timer
			}
			else if(type == TAL_GetBoth)
			{
				if((useraddr == NULL) || (serveraddr == NULL))
				{
					debug("when need both address,ethier is NULL,pUaddr=%lu,pSaddr=%lu",(unsigned long)useraddr,(unsigned long)serveraddr);
					pthread_mutex_unlock(&active_list_lock);
					return NULL;
				}
				memcpy(useraddr,&(au->useraddr),sizeof(struct sockaddr_in));
				memcpy(serveraddr,&(au->serveraddr),sizeof(struct sockaddr_in));
				//虽然不知道是不是客户端传送的信息，但是传送此类信息，也意味着客户端将结束，所以不改timer
			}
			break;
		}
		au = au->next;
	}
	//解锁
	pthread_mutex_unlock(&active_list_lock);

	return au;
}

//更新活跃列表里的服务器信息，主要用于服务器报告自身信息时候，ip和port都已经是网络地址格式
int updateActiveServer(char *account,uint32 ip,uint16 port)
{
		if((NULL == account) || (ip == (uint32)0) || (port == (uint16)0))
		{
				debug("update active server error:pAccount = %s,IP = %lu,Port = %2u",account,(long unsigned int)ip,(unsigned short)port);
				return -1;
		}

		pthread_mutex_lock(&active_list_lock);
		ActiveUser *au = ActiveList->next, *preNode = ActiveList;
		while((au != NULL) && (au->timer != 0))
		{
			if(!memcmp(au->account,account,strlen(account)))  //服务器账号相同，则修改服务器ip和port信息
			{
				au->serveraddr.sin_addr.s_addr = ip;
				au->serveraddr.sin_port = port;
			}
			preNode = au;
			au = preNode->next;
		}
		pthread_mutex_unlock(&active_list_lock);

		return 0;
}

//删除活跃用户列表节点
int deleteActiveUser(char *username)
{
	if(NULL == username)
	{
		debug("delete Active User error,username = NULL");
		return -1;
	}

	pthread_mutex_lock(&active_list_lock);
	ActiveUser *au = ActiveList->next, *preNode = ActiveList;
	while((au != NULL) && (au->timer != 0))
	{
		if(!memcmp(au->username,username,strlen(username)))  //用户名相同，则置timer为0即可，删除由专门线程删除
		{
			au->timer = 0;
			break;
		}
	}
	pthread_mutex_unlock(&active_list_lock);

	return 0;
}

//轮询列表，各节点timer减一，timer已为零则删除
void * pollActiveList()
{
	//死循环
	while(1)
	{
		//休眠固定时间
		struct timeval tv;
		tv.tv_sec = SLEEPTIME;
		tv.tv_usec = 0;
		select(0,NULL,NULL,NULL,&tv);
		/* 轮询链表 */
		//需要加锁,防止搜索的时候有其他处理
		pthread_mutex_lock(&active_list_lock);
		ActiveUser *preAu = ActiveList, *au = preAu->next;
	
		//循环搜索匹配节点
		while(au != NULL)
		{
			//定时器减一
			au->timer--;
			if(au->timer <= 0)  //要删除节点
			{
				//此处不能够直接调用delete方法，因为在解锁和加锁的中间有时间差，可能会有问题
				//从列表上删除该节点
				preAu->next = au->next;
				//清空数值并释放空间,也在锁内是为了防止清空释放时该节点正被使用
				memset(au,0,sizeof(ActiveUser));
				free(au);
				au = preAu->next;  //重新设置au
				continue;
			}
			preAu = au;
			au = preAu->next;
		}
		//解锁
		pthread_mutex_unlock(&active_list_lock);
	}
}



