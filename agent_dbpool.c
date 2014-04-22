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
  pthread_mutex_init(&(dbpool->db_busylock),NULL);
  pthread_cond_init(&(dbpool->dbcond),NULL);
  
  //建立max_size个空闲节点
  bytesize = sizeof(dbList);
  dbIdleList *preNode = NULL,*curNode = NULL;
  dbpool->idlelist = NULL;
  MYSQL *conn = NULL;
  int i = 0;
  for(;i < max_size;i++)
  {
    curNode = (dbList *)malloc(bytesize);
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
  }

  //循环完后i == maxsize
  dbpool->idle_size = i;
  dbpool->busylist = NULL;   //初始时候忙碌列表为空
  
  return i;
}

//获取空闲链接如果空闲列表不为空
MYSQL* getIdleConn()
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
  dbIdleList * tmp = dbpool->idlelist;
  dbpool->idlelist = dbpool->idlelist->next;
  tmp->next = NULL;
  dbpool->idle_size--;
  
  pthread_mutex_unlock (&(dbpool->db_idlelock));
 
  //节点加入忙碌列表,如果没插入成功，则释放该节点资源
  if(inBusyList(tmp) != 0)
  {
    if(tmp == NULL)
	    debug("add in busy list,tmp == NULL");
    debug("in busy list error");
    mysql_close(tmp->db_link);
    free(tmp);
    return NULL;
  }
  
  //debug("sql get link : %d    && in busy list ok\n",(int)tmp->db_link);

  return tmp->db_link;
}

//插入忙碌列表
int inBusyList(dbBusyList *dbl)
{
  if(dbl == NULL)
  {
	debug("in busy list error,pDbl=%lu",(unsigned long)dbl);
    return -1;
  }
  
  dbl->next = NULL;

  pthread_mutex_lock (&(dbpool->db_busylock));
  
  //如果忙碌表为空，直接作为第一个元素
  if(dbpool->busylist == NULL)
  {
     dbpool->busylist = dbl;
     pthread_mutex_unlock (&(dbpool->db_busylock));
     return 0;
  }
  //如果忙碌表不为空,插入作为第一个元素
  dbBusyList *tmp = dbpool->busylist;
  dbpool->busylist = dbl;
  dbl->next = tmp;
  
  pthread_mutex_unlock (&(dbpool->db_busylock));
  
  return 0;
}

//插入空闲列表
int inIdleList(dbIdleList *dil)
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
    return -1;
  }
  //如果空闲列表不为空，插入作为第一个元素
  dbIdleList *tmp = dbpool->idlelist;
  dbpool->idlelist = dil;
  dil->next = tmp;
  dbpool->idle_size++;
  /*空闲列表不为空的时候不需要signal*/
  pthread_mutex_unlock (&(dbpool->db_idlelock));
  
  return 0;
}

//回收链接
int recycleConn(MYSQL *link)
{
  if(link == NULL)
  {
	debug("recycle link error,link=%lu",(unsigned long)link);
    return -1;
  }
  
  pthread_mutex_lock (&(dbpool->db_busylock));
  
  //debug("sql recycle link : %d\n",(int)link);

  //获得该节点的前一个节点
  dbBusyList *preNode = NULL, *curNode = NULL;
  int tmp = getPreNode(dbpool->busylist,link,&preNode);
  //debug("recycle Conn tmp = %d, preNode = %d\n",tmp,(int)preNode); //preNode可能为NULL
  if(tmp != 0)//列表中无该节点
  {
    pthread_mutex_unlock (&(dbpool->db_busylock));
	debug("in recycle conn,there is no tmp");
    //释放该sql链接
    mysql_close(link);
    return -1;
  }
  else //列表中有该节点
  {
    if(preNode == NULL)  //该节点为第一个节点，无前一节点
    {
      curNode = dbpool->busylist;
      dbpool->busylist = curNode->next;    //删除当前节点
    }
    else
    {
      curNode = preNode->next;
      preNode->next = curNode->next;   //删除当前节点
    }
  }
  
  pthread_mutex_unlock (&(dbpool->db_busylock));
  curNode->next = NULL;
  //节点加入空闲列表,如果没插入成功，则释放该节点资源
  if(inIdleList(curNode) != 0)
  {
    debug("in idle list error");
    mysql_close(curNode->db_link);
    free(curNode);
    return -1;
  }
  //debug("in idle list success\n");

  return 0;
}

//该函数实现时不用加锁，依赖调用函数加锁,该函数可改写
int getPreNode(dbList *dblist,MYSQL *link,dbList **ptrptrpreNode)
{
  if((dblist == NULL) || (link == NULL))   //不能允许任何一个为空
  {
	debug("get pre node error,pDblist=%lu,pLink=%lu",(unsigned long)dblist,(unsigned long)link);
    return -1;
  }
  //debug("get preNode : dblist->link = %d\n",(int)(dblist->db_link));
  //把给定的dblist当做链表的第一个节点
  dbList *dl = dblist;
  
  *ptrptrpreNode = NULL;
  //比较所有节点的next
  while(dl != NULL)
  {
    if(dl->db_link == link)
    	return 0;
    *ptrptrpreNode = dl;
    dl = (*ptrptrpreNode)->next;
  }
  
  //走到这一步说明到最后一个节点了
  *ptrptrpreNode = NULL;
  return -1;
}



