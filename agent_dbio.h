
#ifndef DBIO_H_
#define DBIO_H_

/**数据库操作函数声明**/

#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <time.h>
#include "agent_dbpool.h"
#include "agent_debug.h"

//服务器信息结构
struct serverinfo{
	char account[20];
	unsigned int ip;
	unsigned short port;
};
typedef struct serverinfo DBServer;

//用户信息结构
struct userinfo{
	char username[20];
	char password[20];
};
typedef struct userinfo DBUser;

/**
 * 获取当前系统时间
 * @return 返回从1970年1月1日到现在的总秒数
 */
uint64 getCurrentTimeSeconds();

/**
 * 判断用户名和密码是否正确
 * @param username 用户名
 * @param password 密码
 * @return 0 正确，其他错误
 */
int checkUser(char *username,char *password);

/**
 * 获取用户所能访问的server账户
 * @param username 用户名
 * @return 服务器account（需要显式free），NULL表示未查到
 */
char *getServerByUser(char *username);

/**
 * 获取服务器的ip，port信息
 * @param account 服务器账号
 * @param DBServer * （需要显式free），NULL表示不存在
 */
DBServer *getServerInfo(char *account);

/**
 * 更新机房服务器的ip和port
 * @param account 要更新的服务器账户
 * @param ip 新的ip
 * @param port 新的port
 * @return 0 更新成功，其他失败
 */
int updatePrivateServer(char *account,ip_t ip,uint16 port);







#endif
