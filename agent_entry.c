//���ļ�Ϊ�����������

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

#define port 3301  //�������˿�

#define max_db_con 50   //������ݿ�������

int main(int argc,char *argv[])
{
	//��ʼ�����ݿ��
	dbpool_init(max_db_con);
	//��ʼ����Ծ�б�
	initActiveList();
	//��ʼ�������б�
	initSendList();
	//������Ծ�б����߳�
	pthread_t tid;
	//pthread_create�Ĳ���һ��Ҫ��tid������segementation fault
	pthread_create(&tid,NULL,&pollActiveList,NULL);
	//���������б����߳�
	pthread_create(&tid,NULL,&pollSendList,NULL);
	//�����������׽���
	struct sockaddr_in serveraddr;
	memset(&serveraddr,0,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;   //ipv4
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);
	//����udp����
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
	fd_set readset;   //socket��
	while(1)
	{       
		FD_ZERO(&readset);  //���
		FD_SET(listenfd,&readset); //�����������
		//��·ת��
		int ret = select(listenfd+1,&readset,NULL,NULL,NULL);
		if(ret <= 0)
			continue;
		//����Ƿ�׼���ɹ�
		ret = FD_ISSET(listenfd,&readset);
		if(ret == 0)
			continue;
		//��ʼ������
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

		//���߳�ִ������
		pthread_t tid;
		pthread_create(&tid,NULL,processCommand,rc);
		pthread_detach(tid);
	}
}
