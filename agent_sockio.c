
//udp套接字读入输出函数实现
#include "agent_sockio.h"



//socket发送函数
uint16 my_sendto(int fd,const void* msg,int len,unsigned int flags,const struct sockaddr* to,socklen_t tolen)
{
    //开始写
//	debug("fd = %d, addr = %d, port = %d\n",fd,((struct sockaddr_in*)to)->sin_addr.s_addr,((struct sockaddr_in*)to)->sin_port);
	uint16 sendlen = sendto(fd,msg,len,flags,to,tolen);

	return sendlen;
}

//socket接收函数
uint16 my_recvfrom(int fd,void *buf,unsigned int flags,struct sockaddr *from,socklen_t *fromlen)
{
	int len = recvfrom(fd,buf,MAXDATALENGTH,flags,from,fromlen);
	if(len == -1)
	{
		printf("error info: %s\n",strerror(errno));
		return 0u;
	}
	return (uint16)len;
}
