//本文件为程序运行入口

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include "agent_sockio.h"
#include "agent_debug.h"
#include "agent_activeuser.h"
#include "agent_dbio.h"
#include "agent_dbpool.h"
#include "agent_sendlist.h"
#include "agent_threadtask.h"

#define port 3301  //服务器端口

#define max_db_con 50   //最大数据库链接数

int main(int argc,char *argv[])
{
	//初始化数据库池
	dbpool_init(max_db_con);
	//初始化活跃列表
	initActiveList();
	//初始化发送列表
	initSendList();
	//建立活跃列表工作线程
	pthread_t tid;
	//pthread_create的参数一定要加tid，否则segementation fault
	pthread_create(&tid,NULL,&pollActiveList,NULL);
	//建立发送列表工作线程
	pthread_create(&tid,NULL,&pollSendList,NULL);
	//建立服务器套接字
	struct sockaddr_in serveraddr;
	memset(&serveraddr,0,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;   //ipv4
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);
	//建立udp链接
	int listenfd = socket(AF_INET,SOCK_DGRAM,0);
	if(listenfd < 0)
	{
		perror("socket error");
		return -1;
	}
	//bind
	if(bind(listenfd,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr))< 0)
	{
		perror("bind error");
		return -1;
	}
	fd_set readset;   //socket读
	while(1)
	{       
		FD_ZERO(&readset);  //清空
		FD_SET(listenfd,&readset); //加入该描述符
		//多路转接
		int ret = select(listenfd+1,&readset,NULL,NULL,NULL);
		if(ret <= 0)
			continue;
		//检查是否准备成功
		ret = FD_ISSET(listenfd,&readset);
		if(ret == 0)
			continue;
		//开始读数据
		RecvCommand *rc = (RecvCommand *)malloc(sizeof(RecvCommand));
		memset(rc,0,sizeof(RecvCommand));
		rc->sockfd = listenfd;
		socklen_t fromlen;
		uint16 len = my_recvfrom(listenfd,rc->buf,0,(struct sockaddr*)&(rc->recv_addr),&fromlen);
		rc->buflen = len;
		printf("recv data :\n",rc->buf);
		for(int i = 0;i < len;i++)
			printf(" %c ",rc->buf[i]);
		printf("\n");
		for(int i = 0;i < len;i++)
			printf(" %x ",rc->buf[i]);
		printf("    recv finished\n");
	//	debug("read data from ip :%s port :%d length :%2u\n ",inet_ntoa(rc->recv_addr.sin_addr),ntohs(rc->recv_addr.sin_port),len);

		//新线程执行任务
		pthread_t tid;
		pthread_create(&tid,NULL,processCommand,rc);
		pthread_detach(tid);
	}
}
