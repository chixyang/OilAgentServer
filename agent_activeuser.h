
#ifndef ACTIVEUSER_H_
#define ACTIVEUSER_H_

/** 活动用户链表操作声明 **/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include "agent_debug.h"

//活动用户数据结构
struct active_user
{
	char username[20];			  //用户名
	struct sockaddr_in useraddr;  //用户网络地址
	char account[20];			  //机房服务器账户
	struct sockaddr_in serveraddr;//机房服务器网络地址
	char timer;					  //计时器，计时器在初始时和收到数据时都为满值，定时减值，当计时器为0时，清除该用户节点
	struct active_user *next;
};
typedef struct active_user ActiveUser;

enum ToAddrLabel{
	TAL_NONE,
	TAL_ToUser,     //当前为发送给用户的信息
	TAL_ToServer,  //当前为发送给服务器的信息
	TAL_GetBoth    //服务器和用户地址都获取
};

/**
 * 初始化活动列表
 */
void initActiveList();

/**
 * 添加活动用户
 * @param username 要添加的用户名
 * @param ptrUaddr 要添加的用户地址指针
 * @param account  要添加的机房服务器账户
 * @param ptrSaddr 要添加的机房服务器地址
 * @return 0 添加成功，否则失败
 */
int addActiveUser(char *username,struct sockaddr_in* ptrUaddr,char *account,struct sockaddr_in* ptrSaddr);

/**
 * 根据用户名找到节点，并加满节点的定时器
 * @param username 节点的用户名
 * @param useraddr 要获取的用户地址
 * @param server 要获取的机房服务器地址
 * @param type 标识获取哪个地址
 * @return 所找到的节点，NULL表示查找失败
 */
ActiveUser *getActiveUserByName(char *username,struct sockaddr_in *useraddr,struct sockaddr_in *serveraddr,enum ToAddrLabel type);

/**
 * 轮询活动用户列表，各节点timer减一，timer小于等于零则删除该节点（该函数供固定线程调用）
 */
void * pollActiveList();




#endif
