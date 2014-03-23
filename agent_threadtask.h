
#ifndef THREADTASK_H_
#define THREADTASK_H_

/** 线程任务函数声明 **/

#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "agent_sockio.h"
#include "agent_sendlist.h"
#include "agent_activeuser.h"
#include "agent_dbio.h"
#include "agent_debug.h"


//接收命令的数据结构
struct recv_command{
	int sockfd;
	struct sockaddr_in recv_addr;
	char buf[MAXDATALENGTH];
	uint16 buflen;
};
typedef struct recv_command RecvCommand;



//协议各字段的长度（字节数）
#define size_SerialNum 8   //序列号长度
#define size_Length 2      //length长度
#define size_Type 1        //类型长度
#define size_Drum 1        //油桶号长度
#define size_XTime 4       //各个时间的长度
#define size_CheckSum 1    //校验码长度
#define size_split 1       //Data各项的分割位(此处为'\0')长度

//分割位
extern const uint8 SPLIT;


//命令类型不能定义为枚举常量，枚举常量为4字节有符号，和无符号数比对会出问题

//报告ip命令类型
extern const uint8 CMD_REPORT;
//登陆请求命令
extern const uint8 CMD_REQLOGIN;
//回复登陆请求命令
extern const uint8 CMD_RPYLOGIN;
//液位查询请求命令
extern const uint8 CMD_REQLEVEL;
//液位查询回复命令
extern const uint8 CMD_RPYLEVEL;
//取消液位实时发送命令
extern const uint8 CMD_CNLLEVEL;
//温度查询请求命令
extern const uint8 CMD_REQTEMP;
//温度查询回复命令
extern const uint8 CMD_RPYTEMP;
//取消温度实时发送命令
extern const uint8 CMD_CNLTEMP;
//开阀记录查询请求命令
extern const uint8 CMD_REQLOCK;
//开发记录查询回复命令
extern const uint8 CMD_RPYLOCK;
//视频请求命令
extern const uint8 CMD_REQVIDEO;
//视频回复命令
extern const uint8 CMD_RPYVIDEO;
//取消视频实时发送命令
extern const uint8 CMD_CNLVIDEO;
//打洞请求命令
extern const uint8 CMD_REQHOLE;
//打洞回复命令
extern const uint8 CMD_RPYHOLE;
//报警通知协议
extern const uint8 CMD_ALARM;
//液位仪出现或退出命令
extern const uint8 CMD_LEVELINOUT;
//温度传感器出现或退出命令
extern const uint8 CMD_TEMPINOUT;
//实时信息请求协议
extern const uint8 CMD_REQREALLT;
//实时信息回复协议
extern const uint8 CMD_RPYREALLT;
//取消液位和温度实时传输协议
extern const uint8 CMD_CNLREALLT;





/**
 * 处理接收的命令,套接字接受完命令后调
 * @param arg 命令
 */
void * processCommand(void * arg);

/**
 * 生成登录回复协议
 * @param sockaddr 被回复的套接字，主要用其ip和port，将这些信息回复给另一方
 * @param serial_num 回复协议头，和发送协议头相同
 * @param *rlplen 生成的回复协议的长度
 * @return 要回复的协议
 */
char * getReplyLogPtl(struct sockaddr_in *sockaddr,char *serial_num,uint16 *rlplen);

/**
 * 获取校验位
 * @param data 要校验的数据
 * @param len 校验数据的总长度
 * @return 校验后的数据序列
 */
char* getCheckSum(char *data,uint16 len);

/**
 * 检查校验码
 * @param data 要校验的数据
 * @param len 校验数据的总长度
 * @return 检查结果，0表示校验成功，其他失败
 */
uint8 checkSum(char *data,uint16 len);








#endif
