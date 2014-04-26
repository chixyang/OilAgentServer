/**
 * 数据库池函数定义文件
 */
 
#ifndef DBPOOL_H
#define DBPOOL_H

#include <mysql/mysql.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "agent_debug.h"

//服务器名
extern const char* server;
//用户名
extern const char* username;
//密码
extern const char* password;
//数据库名
extern const char* database;


//忙碌表结构
struct db_node
{
  MYSQL * db_link;
  struct db_node *next;
};

typedef struct db_node DBNode;


//数据库池结构
struct DBpool
{
  pthread_mutex_t db_idlelock;   //空闲表互斥锁
  pthread_cond_t dbcond;    //条件
  DBNode *idlelist;     //空闲列表
  int idle_size;            //空闲列表大小,专门加这个值是为了确保程序的正确性，还可以帮助系统分析是否添加数据库链接
};


//数据库池节点
extern struct DBpool *dbpool;


/**
 * 初始化数据库池，建立max_size个链接
 * @param max_size 所要建立的最大链接个数
 * @return 建立的链接个数，-1表示建立出错
 */
int dbpool_init(int max_size);

/**
 * 获取空闲sql链接，若当前无空闲sql链接，则该函数阻塞，等待有的时候再返回
 * @return 空闲的数据库节点，里面包含MYSQL链接,NULL表示出错
 */
DBNode* getIdleConn();

/**
 * 将节点插入空闲列表
 * @param dil 待插入的节点
 * @return 0表示插入成功，其他表示不成功
 */
int inIdleList(DBNode *dil);

/**
 * 数据库节点的回收
 * @param link 待回收的节点
 * @return 0表示回收成功，其他表示回收失败
 */
int recycleConn(DBNode *link);

#endif
