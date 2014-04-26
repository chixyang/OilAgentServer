/**
 * 数据库池函数实现文件
 */

#include "agent_dbpool.h"

const char *server = "localhost";
const char *username = "root";
const char *password = "123456";
const char *database = "OilAgentDB";

struct DBpool *dbpool;


//初始化数据库池
int dbpool_init(int max_size)
{
  if(max_size == 0)
  {
	debug("db error:max_size = 0");
    return -1;
  }
    
  int bytesize = sizeof(struct DBpool);
  dbpool = (struct DBpool*)malloc(bytesize);  
  //先都初始化为0
  memset(dbpool,0,bytesize);
  
  //初始化互斥和条件变量,动态变量只能使用init函数初始化，不能使用INITIALIZER
  pthread_mutex_init(&(dbpool->db_idlelock),NULL);
  pthread_cond_init(&(dbpool->dbcond),NULL);
  
  //建立max_size个空闲节点
  bytesize = sizeof(DBNode);
  DBNode *preNode = NULL,*curNode = NULL;
  dbpool->idlelist = NULL;
  MYSQL *conn = NULL;
  int i = 0;
  for(;i < max_size;i++)
  {
    curNode = (DBNode *)malloc(bytesize);
    memset(curNode,0,bytesize);   //初始化
    conn = mysql_init((MYSQL *)NULL);
    if(conn != mysql_real_connect(conn, server, username, password, database, 3306, NULL, 0)) 
    {
      	debug("create mysql connection error : %s",mysql_error(conn));
      	mysql_close(conn);
      	conn = NULL;
      	free(curNode);
      	curNode = NULL;
		if(i == (--max_size))  //如果是最后一次循环出的问题，则i减1,同时只要出问题max_size都会减1
			i--;
      	continue;
    }
    curNode->db_link = conn;
    //第一个节点给节点头
    if(!i)
    {
      	dbpool->idlelist = curNode;
		preNode = dbpool->idlelist;
		continue;
    }
    //非第一个节点
    preNode->next = curNode;
    preNode = curNode;
	curNode->next = NULL;  //再确认一下最后一个节点的next是null
  }

  //循环完后i == maxsize
  dbpool->idle_size = i;
  
  return i;
}

//获取空闲链接如果空闲列表不为空
DBNode* getIdleConn()
{
  /*这里的lock和wait最好换成等待一定时间的函数，因为不能永远等下去，尤其是tcp链接的时候，最多等到某个时间，否则退出等待*/
  pthread_mutex_lock (&(dbpool->db_idlelock));
  
  while((dbpool->idle_size == 0))  //数据库池或者空闲列表为空
     pthread_cond_wait(&(dbpool->dbcond),&(dbpool->db_idlelock));
  
  //空闲表出现错误
  if(dbpool->idlelist == NULL)
  {
     debug("in dbpool ,idlelist == NULL\n");	  
     pthread_mutex_unlock (&(dbpool->db_idlelock));
     return NULL;
  }
  //有空链接,取出第一个节点使用
  DBNode * tmp = dbpool->idlelist;
  dbpool->idlelist = dbpool->idlelist->next;
  dbpool->idle_size--;
  
  pthread_mutex_unlock (&(dbpool->db_idlelock));
  
  //debug("sql get link : %d    && in busy list ok\n",(int)tmp->db_link);

  tmp->next = NULL;  //这个一定要有，避免出错
  return tmp;
}

//插入空闲列表
int inIdleList(DBNode *dil)
{
  if(dil == NULL)
  {
	debug("in idle list error,pDil=%lu",(unsigned long)dil);
    return -1;
  }
  
  dil->next = NULL;

  pthread_mutex_lock (&(dbpool->db_idlelock));
  
  //如果空闲列表为空，直接作为第一个元素
  if(dbpool->idle_size == 0)
  {
    dbpool->idle_size++;
    dbpool->idlelist = dil; 
    pthread_mutex_unlock (&(dbpool->db_idlelock));
    //唤醒等待程序
    pthread_cond_signal (&(dbpool->dbcond));
    return 0;
  }
  //空闲列表出错
  if(dbpool->idlelist == NULL)
  {
    pthread_mutex_unlock (&(dbpool->db_idlelock));
    debug("idlelist error");
	//出错则释放，在此处释放好于在recycle里面释放，因为这是内部函数，对出错原因了解的更清楚
	mysql_close(dil->db_link);
	free(dil); 
    return -1;
  }
  //如果空闲列表不为空，插入作为最后一个元素
  DBNode *tmp = dbpool->idlelist;
  while(tmp->next)
		  tmp = tmp->next;
  tmp->next = dil;
  dbpool->idle_size++;
  /*空闲列表不为空的时候不需要signal*/
  pthread_mutex_unlock (&(dbpool->db_idlelock));
  
  return 0;
}

//回收链接
int recycleConn(DBNode *link)
{
  if(link == NULL)
  {
	debug("recycle link error,link=%lu",(unsigned long)link);
    return -1;
  }
  
  //debug("sql recycle link : %d\n",(int)link);
  //节点加入空闲列表,如果没插入成功，则释放该节点资源
  if(inIdleList(link) != 0)
  {
    debug("in idle list error");
    return -1;
  }
  //debug("in idle list success\n");

  return 0;
}


