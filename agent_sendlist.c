
//发送列表要将所有的信息都复制过来，字符数组也要malloc个新的，以免原来的被被释放掉，影响发送
#include "agent_sendlist.h"

struct send_list *SendList;

//初始化函数
void initSendList()
{
	SendList = (struct send_list *)malloc(sizeof(struct send_list));
	SendList->head = (SendNode *)malloc(sizeof(SendNode));
	memset(SendList->head,0,sizeof(SendNode));
	SendList->node_num = 0;
	pthread_mutex_init(&(SendList->send_list_lock),NULL);
	pthread_cond_init(&(SendList->send_list_cond),NULL);
}

//向队列中加入新发送节点
int InSendQueue(int fd,struct sockaddr_in *addr,char *buf,uint16 len)
{
	//新建发送节点
	SendNode *sn = (SendNode *)malloc(sizeof(SendNode));
	memset(sn,0,sizeof(SendNode));
	sn->sockfd = fd;
	memcpy(&(sn->sendaddr),addr,sizeof(struct sockaddr_in));
	sn->buf = (char *)malloc(len *sizeof(char));
	memcpy(sn->buf,buf,len);
	sn->buflen = len;
	sn->next = NULL;
	
	//将节点添加到发送列表中
	pthread_mutex_lock(&(SendList->send_list_lock));
	//将新节点添加作为最后一个节点
	SendNode *node = SendList->head;
	while(node->next)
		node = node->next;
	node->next = sn;
	SendList->node_num++;
	//判断是否为唯一一个节点
	if(SendList->node_num == 1)
	{
		pthread_mutex_unlock(&(SendList->send_list_lock));
		pthread_cond_signal(&(SendList->send_list_cond));
		return 0;
	}
	pthread_mutex_unlock(&(SendList->send_list_lock));
	return 0;
}

//线程轮询队列，发送并删除节点
void *pollSendList()
{
	while(1)
	{
		pthread_mutex_lock(&(SendList->send_list_lock));
		//循环等待条件满足
		while(SendList->node_num == 0)
			pthread_cond_wait(&(SendList->send_list_cond),&(SendList->send_list_lock));
		SendNode *sn = SendList->head->next;
		SendList->head->next = sn->next;   //从列表上删除节点
		SendList->node_num--;
		pthread_mutex_unlock(&(SendList->send_list_lock));
		//根据节点信息发送数据
		my_sendto(sn->sockfd,sn->buf,sn->buflen,0,(struct sockaddr*)&(sn->sendaddr),sizeof(struct sockaddr_in));
		char type = *(char *)(sn->buf+8+2);
		debug("sendto ip :%s,port :%d\n, type :%x\n,send buf :%s,buf len :%d\n",inet_ntoa(sn->sendaddr.sin_addr),ntohs(sn->sendaddr.sin_port),type,sn->buf,sn->buflen);

		free(sn->buf);
		free(sn);
	}
}


