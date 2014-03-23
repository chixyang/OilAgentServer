
/** 线程任务处理函数实现 **/

#include "agent_threadtask.h"


//由于inet_ntoa函数使用的是静态区，所以需要加锁以使多线程安全访问
pthread_mutex_t ntoa_lock = PTHREAD_MUTEX_INITIALIZER;

//分割位
const uint8 SPLIT = 0x00;

//报告ip命令类型
const uint8 CMD_REPORT = 0x00;
//登陆请求命令
const uint8 CMD_REQLOGIN = 0x01;
//回复登陆请求命令
const uint8 CMD_RPYLOGIN = 0x02;
//液位查询请求命令
const uint8 CMD_REQLEVEL = 0x03;
//液位查询回复命令
const uint8 CMD_RPYLEVEL = 0x04;
//温度查询请求命令
const uint8 CMD_REQTEMP = 0x05;
//温度查询回复命令
const uint8 CMD_RPYTEMP = 0x06;
//开阀记录查询请求命令
const uint8 CMD_REQLOCK = 0x07;
//开发记录查询回复命令
const uint8 CMD_RPYLOCK = 0x08;
//视频请求命令
const uint8 CMD_REQVIDEO = 0x09;
//视频回复命令
const uint8 CMD_RPYVIDEO = 0x10;
//取消视频实时发送命令
const uint8 CMD_CNLVIDEO = 0xF9;
//打洞请求命令
const uint8 CMD_REQHOLE = 0xFE;
//打洞回复命令
const uint8 CMD_RPYHOLE = 0xFF;
//报警通知协议
const uint8 CMD_ALARM = 0xF0;
//液位仪出现或退出命令
const uint8 CMD_LEVELINOUT = 0xF1;
//温度传感器出现或退出命令
const uint8 CMD_TEMPINOUT = 0xF2;
//实时信息请求协议
const uint8 CMD_REQREALLT = 0x0A;
//实时信息回复协议
const uint8 CMD_RPYREALLT = 0x0B;
//取消液位和温度实时传输协议
const uint8 CMD_CNLREALLT = 0xFA;

//任务处理函数
void *processCommand(void *arg)
{
	RecvCommand *cmd = (RecvCommand *)arg;
	
	/* 校验数据长度是否对的上，校验码是否正确 */
	char length[size_Length];
	memcpy(length,cmd->buf + size_SerialNum,size_Length);

	if((cmd->buflen - size_SerialNum) != *((uint16 *)length))  //长度对不上
	{
			free(arg);
			return NULL;
	}
	
	//检验校验码
	uint8 CheckSum = checkSum(cmd->buf,cmd->buflen);
	if(CheckSum) //校验错误
	{
			free(arg);
			return NULL;
	}
	
	//正确协议
	uint8 type = *(uint8 *)(cmd->buf + size_SerialNum + size_Length);
	debug("agent recv type : %x\n",type);
	if(CMD_REPORT == type)   //机房服务器报告自己信息
	{
		char *account = cmd->buf + size_SerialNum + size_Length + size_Type;
		//debug("server account: %s\n",account);
		//debug("server ip network : %s port : %hu\n",inet_ntoa(cmd->recv_addr.sin_addr),ntohs(cmd->recv_addr.sin_port));
		updatePrivateServer(account,cmd->recv_addr.sin_addr.s_addr,cmd->recv_addr.sin_port);
	}
	else if(CMD_REQLOGIN == type) //用户登录服务器
	{	char *username = cmd->buf + size_SerialNum + size_Length + size_Type;
		char *password = cmd->buf + size_SerialNum + size_Length + size_Type + strlen(username) + size_split;
		//debug("username : %s  password : %s\n",username,password);
		ip_t ip = 0;       //机房服务器ip
		uint16 port = 0;   //机房服务器port
		if(!checkUser(username,password)) //用户名密码正确
		{
		//	debug("check user ok!");
			//获得机房服务器ip和port
			char *account = getServerByUser(username); //account需要显式释放
		//	debug("account : %s\n",account);
			DBServer *server = getServerInfo(account); //server需显式释放
		//	debug("after get server info\n");
			if(server == NULL)
			{
				debug("account error\n");
				return NULL;
			}
			ip = server->ip;    //已经是网络地址格式
			port = server->port;
			free(server);
			//建立服务器套接字地址
			struct sockaddr_in serveraddr;
			memset(&serveraddr,0,sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_addr.s_addr = ip;
			serveraddr.sin_port = port;
		//	debug("before add active user\n");
			//新加活跃用户节点
			addActiveUser(username,&(cmd->recv_addr),account,&serveraddr);
			free(account);
			//向机房服务器发送客户端信息
			//首先建立协议
			uint16 rlplen = 0;
			//向服务器发送客户端ip和port,加入发送列表
		//	debug("before get reply login\n");
			char *rlp = getReplyLogPtl(&(cmd->recv_addr),cmd->buf,&rlplen);  //rlp需要显示释放
			InSendQueue(cmd->sockfd,&serveraddr,rlp,rlplen);
			free(rlp);
		}
		//向客户端发送服务器ip和port
		//建立服务器套接字地址
		struct sockaddr_in serveraddr;
		memset(&serveraddr,0,sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = ip;   //若未验证成功，ip和port都为0
		serveraddr.sin_port = port;
		//首先建立协议
		uint16 rlplen = 0;
		//向客户端发送服务器ip和port,加入发送列表
		char *rlp = getReplyLogPtl(&serveraddr,cmd->buf,&rlplen);  //rlp需要显示释放
		InSendQueue(cmd->sockfd,&(cmd->recv_addr),rlp,rlplen);
		free(rlp);
	}
	else if((CMD_REQLEVEL == type) || (CMD_REQTEMP == type) || (CMD_REQLOCK == type) || (CMD_REQVIDEO == type) || (CMD_REQREALLT == type))   //这些都是客户端发来的请求
     	{	//获取用户名
		char *username = cmd->buf + size_SerialNum + size_Length + size_Type;
		struct sockaddr_in serveraddr;
		memset(&serveraddr,0,sizeof(struct sockaddr_in));
		//检查是否客户端已经登录
		ActiveUser *au = getActiveUserByName(username,NULL,&serveraddr,TAL_ToServer);
		debug("reply au is : %d\n",(int)au);
		if(au != NULL) //表示已登录，未登录则不管，即丢掉该包
		{
			InSendQueue(cmd->sockfd,&(serveraddr),cmd->buf,cmd->buflen);
			//更新au中的客户端信息
			addActiveUser(username,&(cmd->recv_addr),au->account,&serveraddr);
		}
	}
	else if((CMD_RPYLEVEL == type) || (CMD_RPYTEMP == type) || (CMD_RPYLOCK == type) || (CMD_RPYVIDEO == type) || (CMD_RPYREALLT == type)) //以上都是来自服务器的请求
	{	//获取用户名
		char *username = cmd->buf + size_SerialNum + size_Length + size_Type;
		struct sockaddr_in useraddr;
		memset(&useraddr,0,sizeof(struct sockaddr_in));
		//检查是否客户端已经登录
		ActiveUser *au = getActiveUserByName(username,&useraddr,NULL,TAL_ToUser);
	//	debug("cmd type = %x\n",type);
		//debug("reply au is : %d\n",(int)au);
		if(au != NULL) //表示已登录，未登录则不管，即丢掉该包
		{
			InSendQueue(cmd->sockfd,&(useraddr),cmd->buf,cmd->buflen);
			addActiveUser(username,&useraddr,au->account,&(cmd->recv_addr));
		}
	}
	else if((CMD_CNLVIDEO == type) || (CMD_ALARM == type) || (CMD_LEVELINOUT == type) || (CMD_TEMPINOUT == type) || (CMD_CNLREALLT == type)) //以上不能确定来自谁的请求
	{	//获取用户名
		char *username = cmd->buf + size_SerialNum + size_Length + size_Type;
		//检查是否客户端已经登录
		struct sockaddr_in serveraddr,useraddr;
		memset(&serveraddr,0,sizeof(struct sockaddr_in));
		memset(&useraddr,0,sizeof(struct sockaddr_in));
		ActiveUser *user = getActiveUserByName(username,&useraddr,&serveraddr,TAL_GetBoth);		     
	//	debug("username: %s,cmd type : %x ,au : %d\n",username,type,(int)au);
		if(user != NULL) //表示已登录，未登录则不管，即丢掉该包
		{
			struct sockaddr_in *addr = NULL;
			//将收到的ip和port与保存的对比
			if((cmd->recv_addr.sin_addr.s_addr == useraddr.sin_addr.s_addr) && (cmd->recv_addr.sin_port == useraddr.sin_port))  //与客户端对比
				addr = &serveraddr;
			else if((cmd->recv_addr.sin_addr.s_addr == serveraddr.sin_addr.s_addr) && (cmd->recv_addr.sin_port == serveraddr.sin_port)) //与服务器对比
				addr = &useraddr;
			if(addr != NULL)
				InSendQueue(cmd->sockfd,addr,cmd->buf,cmd->buflen);
		}
	}

	free(arg);
	return NULL;

}


//生成登陆回复协议
char * getReplyLogPtl(struct sockaddr_in *sockaddr,char *serial_num,uint16 *rlplen)
{
	//填充数据位
	char data[20];    //ip和port的字节表示加起来不超过20字节
	memset(data,0,20);
	//inet_ntoa函数最好用锁加起来，因为指向的是同一静态空间，多个调用会改写
	pthread_mutex_lock(&(ntoa_lock));
	char *addr = inet_ntoa(sockaddr->sin_addr);
	uint16 len = strlen(addr);
	memcpy(data,addr,len);
	pthread_mutex_unlock(&(ntoa_lock));
	data[len] = SPLIT;   //设置分隔符
	len++;
	sprintf(data+len,"%hu",ntohs(sockaddr->sin_port));
	len += strlen(data+len);
	data[len] = SPLIT;   //data尾设置分隔符
	len++;   //len 即为data长度
	uint16 datalen = len;
	//len为协议总长度
	len += size_SerialNum + size_Length + size_Type + size_CheckSum;
	//申请协议空间
	char *rlp = (char *)malloc(len * sizeof(char));
	memset(rlp,0,len);
	//设置SerialNum
	memcpy(rlp,serial_num,size_SerialNum);
	//设置Length
	*((uint16 *)(rlp + size_SerialNum)) = len - size_SerialNum;
	//设置Type
	*((uint8 *)(rlp + size_SerialNum + size_Length)) = CMD_RPYLOGIN;
	//设置Data
	memcpy((rlp + size_SerialNum + size_Length + size_Type),data,datalen);
	//设置Checksum
	getCheckSum(rlp,len);
	//设置协议总长度
	*rlplen = len;

	return rlp;
}

//获取校验位
char* getCheckSum(char *data,uint16 len)
{
	data[len -1] = 0;
	for(int i = 0; i < len - 1; i++)
		data[len - 1] ^= data[i];

	return data;	
}

//检查校验码
uint8 checkSum(char *data,uint16 len)
{
	char checkByte = data[0];
	for(int i = 1; i < len; i++)
		checkByte ^= data[i];

	return checkByte;
}

