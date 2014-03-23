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
struct DBList
{
  MYSQL * db_link;
  struct DBList *next;
};

typedef struct DBList dbList;

//忙碌列表类型
typedef dbList dbBusyList;
//空闲列表类型
typedef dbList dbIdleList;


//数据库池结构
struct DBpool
{
  pthread_mutex_t db_idlelock;   //空闲表互斥锁
  pthread_mutex_t db_busylock;   //忙碌表互斥锁
  pthread_cond_t dbcond;    //条件
  dbBusyList *busylist;     //忙碌列表
  dbIdleList *idlelist;     //空闲列表
  int idle_size;            //空闲列表大小,专门加这个值是为了确保程序的正确性，还可以帮助系统分析是否添加数据库链接
  int db_shutdown;          //关闭标识符，0表示线程池正常使用，1表示线程池撤销
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
 * @return 空闲的MYSQL链接,NULL表示出错
 */
MYSQL* getIdleConn();

/**
 * 将节点插入忙碌列表
 * @param dbl 待插入的节点
 * @return 0表示插入成功，其他表示插入不成功
 */
int inBusyList(dbBusyList *dbl);

/**
 * 将节点插入空闲列表
 * @param dil 待插入的节点
 * @return 0表示插入成功，其他表示不成功
 */
int inIdleList(dbIdleList *dil);

/**
 * sql链接的回收
 * @param link 待回收的节点
 * @return 0表示回收成功，其他表示回收失败
 */
int recycleConn(MYSQL *link);

/**
 * 在链表中获取某个节点的前一个节点，该函数必须在dbpool->db_busylock或者dbpool->db_idlelock之间使用，否则是不安全的
 * @param dblist 提供的链表头
 * @param link 要查询的mysql链接
 * @param preNode 返回的前一个节点的指针的指针（注意：这里调试好久才找到原因）
 * @return 0 表示查到节点，若此时preNode为NULL，则查到的节点为第一个节点，无preNode，否则为查到节点的前一个节点
 *         返回其他表示未查到节点
 */
int getPreNode(dbList *dblist,MYSQL *link,dbList **preNode);

/**
 * 向链接池中添加新链接
 * @param add_num 需要添加的数目
 * @return 已添加的数目
 */
int dbpool_add_dblink(int add_num);

/**
 * 销毁链接池,如果忙碌列表不为空，则等待忙碌列表为空后进行销毁
 * @return 0 表示销毁成功，其他表示已经被销毁
 */
int dbpool_destroy();



#endif
