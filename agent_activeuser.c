
/**活动用户链表定义及操作**/

#include "agent_activeuser.h"



#define TIMER 6     //轮询次数为6次，每次半分钟

#define SLEEPTIME 30  //30s轮询一次

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
		return -1;

	//先轮训查找节点是否已经存在，若存在则查看地址是否相同，相同则返回，不同删除原来地址，如不存在则直接新建
	pthread_mutex_lock(&active_list_lock);
	ActiveUser *au = ActiveList->next, *preNode = ActiveList;
	while(au != NULL)
	{
		if(!memcmp(au->username,username,strlen(username)))  //用户名相同
		{
			if((ptrUaddr->sin_addr.s_addr == au->useraddr.sin_addr.s_addr) && (ptrUaddr->sin_port == au->useraddr.sin_port) && (ptrSaddr->sin_addr.s_addr == au->serveraddr.sin_addr.s_addr) && (ptrSaddr->sin_port == au->serveraddr.sin_port)) //双方地址也相同,即不用加入了
			{
				debug("duplicate node in active user list");
				pthread_mutex_unlock(&active_list_lock);
				return 0;
			}
			//双方节点不相同，删除原节点
			else
				au->timer = 0;   //设置timer为0,到时自动删除
			break;
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

//删除节点
int deleteActiveUser(ActiveUser *preAu)
{
	if(preAu == NULL)
		return -1;
	
	//需要加锁
	pthread_mutex_lock(&active_list_lock);
	//删除传递节点的后一个节点
	ActiveUser *au = preAu->next;
	//从列表上删除该节点
	preAu->next = au->next;
	//清空数值并释放空间,也在锁内是为了防止清空释放时该节点正被使用
	memset(au,0,sizeof(ActiveUser));
	free(au);
	//解锁
	pthread_mutex_unlock(&active_list_lock);

	return 0;
}

//根据用户名找到某个节点
ActiveUser *getActiveUserByName(char *username,struct sockaddr_in *useraddr,struct sockaddr_in *serveraddr,enum ToAddrLabel type)
{
	if((username == NULL) || ((useraddr == NULL) && (serveraddr == NULL)) || (type == TAL_NONE))
		return NULL;

	//需要加锁,防止搜索的时候有节点被删除
	pthread_mutex_lock(&active_list_lock);
	ActiveUser *au = ActiveList->next;
	//debug("username : %s\n",username);
	//debug("au is : %d\n",(int)au);
	//循环搜索匹配节点
	while(au != NULL)
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
					return NULL;
				}
				memcpy(serveraddr,&(au->serveraddr),sizeof(struct sockaddr_in));
				au->timer = TIMER;   //加满节点时间，放于锁内可防止节点出锁即被删除(调用此方法就意味着一定会发送信息给对方)
			}
			else if(type == TAL_ToUser)
			{
				if(useraddr == NULL)
				{
					pthread_mutex_unlock(&active_list_lock);
					return NULL;
				}
				memcpy(useraddr,&(au->useraddr),sizeof(struct sockaddr_in));  //服务器传送信息并不一定表明客户端依然在连接，所以不修改timer
			}
			else if(type == TAL_GetBoth)
			{
				if((useraddr == NULL) || (serveraddr == NULL))
				{
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

//轮询列表，各节点timer减一，timer已为零则删除
void * pollActiveList()
{
	//死循环
	while(1)
	{
		//休眠固定时间，sleep线程安全，不休眠进程
		sleep(SLEEPTIME);
		/* 轮询链表 */
		//需要加锁,防止搜索的时候有节点被删除
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



