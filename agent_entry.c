//���ļ�Ϊ�����������

#include "agent_sockio.h"
#include "agent_debug.h"
#include "agent_activeuser.h"
#include "agent_dbio.h"
#include "agent_dbpool.h"
#include "agent_sendlist.h"
#include "agent_threadtask.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
//#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>
#include <unistd.h>

#define port 3301  //�������˿�

#define max_db_con 20   //������ݿ�������


int main(int argc,char *argv[])
{
	//��ʼ���ػ�����
	struct rlimit rl;
	//�����ļ�����
	umask(0);
	//��ȡϵͳ����ļ�������
	if(getrlimit(RLIMIT_NOFILE,&rl) < 0)
	{
		perror("can not get file limit\n");
		exit(1);
	}
	//��Ϊ�Ự�׽��̣����ն�ʧȥ��ϵ
	pid_t pid;
	if((pid = fork()) < 0)
	{
		perror("can not fork\n");
		exit(1);
	}
	else if(pid != 0) //������
		exit(0);
	setsid();
	//�������ն�����ź�
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGHUP,&sa,NULL) < 0)
	{
		perror("can not ignore SIGHUP");
		exit(1);
	}
	if((pid = fork()) < 0)
	{
		perror("can not fork");
		exit(1);
	}
	else if(pid != 0)  //������
		exit(0);
	//�ı乤��Ŀ¼��root�����ļ�ϵͳ���������,�ػ�������ҪrootȨ��
	if(chdir("/") < 0)
	{
		perror("can not change directory");
		exit(1);
	}
	//�ر����е��ļ�������
	if(rl.rlim_max == RLIM_INFINITY)
			rl.rlim_max = 1024;
	int i = 0;
	for(;i < rl.rlim_max;i++)
			close(i);
	//���ļ�������0��1����/dev/null
	int fd0,fd1,fd2;
	fd0 = open("/dev/null",O_RDWR);
	fd1 = dup(0);

	//��ʼ����־�ļ�������2������־�ļ�
	fd2 = initLog();
	fd2 = dup2(fd2,2);
	
	if(fd0 != 0 || fd1 != 1 || fd2 != 2)
	{
		debug("unexpected file descriptions %d %d %d",fd0,fd1,fd2);
		exit(1);
	}

	debug("^^^^^OilAgentServer Start^^^^^");
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
		debug("socket error");
		return -1;
	}
	//bind
	if(bind(listenfd,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr))< 0)
	{
		debug("bind error");
		return -1;
	}
	fd_set readset;   //socket��
	debug("Start Read Request and Process");
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
		if(len <= 0)
		{
				debug("recv error ,len = %d",len);
				free(rc);
				continue;
		}
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
