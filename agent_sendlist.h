
#ifndef SENDLIST_H_
#define SENDLIST_H_
//发送列表函数声明

#include "agent_debug.h"
#include "agent_sockio.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

//发送节点
struct send_node {
	int sockfd;
	struct sockaddr_in sendaddr;
	char *buf;
	uint16 buflen;
	struct send_node *next;
};
typedef struct send_node SendNode;

//发送列表
struct send_list{
	SendNode *head;   //带一个头节点
	uint16 node_num;
	pthread_mutex_t send_list_lock;
	pthread_cond_t send_list_cond;
};

extern struct send_list *SendList;

/**
 * 初始化发送列表
 */
 void initSendList();
 
 /**
  * 向队列中加入新节点
  * @param fd 当前使用的发送描述符
  * @param addr 目的地址
  * @param buf 要发送的数据
  * @param len 发送数据的长度
  * @return 0表示加入成功，其他失败
  */
int InSendQueue(int fd,struct sockaddr_in *addr,char *buf,uint16 len);

/**
 * 线程轮询任务，等待、发送、删除节点
 */
 void *pollSendList();


#endif
