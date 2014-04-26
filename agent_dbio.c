
/**数据库操作函数实现**/

#include "agent_dbio.h"


//设置字符编码为utf8
#define mysql_setUTF8(x)    do{                                          	\
                              if(mysql_query(x->db_link,"set names \'utf8\'"))     	\
	                                 {                                      \
								debug("set utf8 error : %s",mysql_error(x->db_link));	 \
		                                    recycleConn(x);                 \
									 }	\
	                          }while(0)

// 获取当前系统时间
uint64 getCurrentTimeSeconds()
{
	return (uint64)(time((time_t *)NULL)); //获取当前国际标准时间，存的是总的秒数，因为time_t其实是int，所以所存储最大时间到2038年
}

//判断用户名和密码是否正确
int checkUser(char *username,char *password)
{
	if((NULL == username) || (NULL == password))
	{
			debug("check user error,pUsername=%lu,pPassword=%lu",(unsigned long)username,(unsigned long)password);
			return -1;
	}
	DBNode *dbconn = getIdleConn();
	MYSQL_RES *res;      //查询的result
	MYSQL_ROW row;
	char *sql_str = NULL;   //sql语句
  
	//设置字符编码为utf8
	mysql_setUTF8(dbconn);
	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select username from UserInfo where username = '%s' and password = '%s'",username,password);
	//执行查询
	if(mysql_query(dbconn->db_link,sql_str))
	{
		debug("query user info error:%s",mysql_error(dbconn->db_link));
		recycleConn(dbconn);
		free(sql_str);
		return -1;
	}
	//获取查询结果
	res = mysql_use_result(dbconn->db_link);
	//如果查询结果不为空
	if((row = mysql_fetch_row(res)) != NULL)
	{
		//释放资源
		mysql_free_result(res);
		recycleConn(dbconn);
		free(sql_str);
		return 0;
	}
	//未查到数据
	mysql_free_result(res);
	recycleConn(dbconn);
	free(sql_str);
	return -1;
}

//获取用户所处的server账户
char *getServerByUser(char *username)
{
	if(NULL == username)
	{
		debug("get server by user error,pUsername=%lu",(unsigned long)username);
		return NULL;
	}
	DBNode *dbconn = getIdleConn();
	MYSQL_RES *res;      //查询的result
	MYSQL_ROW row;
	char *sql_str = NULL;   //sql语句
  
	//设置字符编码为utf8
	mysql_setUTF8(dbconn);
	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select account from UserInServer where username = '%s'",username);
	//执行查询
	if(mysql_query(dbconn->db_link,sql_str))
	{
		debug("get server by user error : %s",mysql_error(dbconn->db_link));
		recycleConn(dbconn);
		free(sql_str);
		return NULL;
	}
	//获取查询结果
	res = mysql_use_result(dbconn->db_link);
	//如果查询结果不为空
	if((row = mysql_fetch_row(res)) != NULL)
	{
		int len = strlen(row[0]);
		char *info = (char *)malloc(len + 1); //加上结束位
		memcpy(info,row[0],len);
		info[len] = '\0'; //加上结束位
		//释放资源
		mysql_free_result(res);
		recycleConn(dbconn);
		free(sql_str);
		return info;
	}
	//未查到数据
	mysql_free_result(res);
	recycleConn(dbconn);
	free(sql_str);
	return NULL;
}

//获取Server的ip和port
DBServer *getServerInfo(char *account)
{
	if(NULL == account)
	{
		debug("get server info error : pAccount=%lu",(unsigned long)account);
		return NULL;
	}
	DBNode *dbconn = getIdleConn();
	MYSQL_RES *res;      //查询的result
	MYSQL_ROW row;
	char *sql_str = NULL;   //sql语句
  
	//设置字符编码为utf8
	mysql_setUTF8(dbconn);
	//设置查询语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"select ip,port from ServerInfo where account = '%s'",account);
	//执行查询
	if(mysql_query(dbconn->db_link,sql_str))
	{
		debug("get serverinfo error : %s",mysql_error(dbconn->db_link));
		recycleConn(dbconn);
		free(sql_str);
		return NULL;
	}
	//获取查询结果
	res = mysql_use_result(dbconn->db_link);
	//如果查询结果不为空
	if((row = mysql_fetch_row(res)) != NULL)
	{
		DBServer *server = (DBServer *)malloc(sizeof(DBServer));
		memset(server,0,sizeof(DBServer));
		server->ip = (ip_t)atoi(row[0]);
		server->port = (uint16)atoi(row[1]);
		memcpy(server->account,account,strlen(account) + 1); //把'\0'copy过去,因为这里的account是查询出来的，所以一定有'\0'

		//释放资源
		mysql_free_result(res);
		recycleConn(dbconn);
		free(sql_str);
		return server;
	}
	//未查到数据
	mysql_free_result(res);
	recycleConn(dbconn);
	free(sql_str);
	return NULL;
}

//更新机房服务器ip和port信息，更新与读互斥问题不管，多几次读请求就好了
int updatePrivateServer(char *account,ip_t ip,uint16 port)
{
	if((NULL == account) || (((ip_t)0) == ip) || (((uint16)0) == port))
	{
		debug("update private server error,pAccount=%lu,Ip=%lu,Port=%hu",((long unsigned)account),ip,port);
		return -1;
	}
	DBNode *dbconn = getIdleConn();
	unsigned long affected_rows = 0;   //改变的语句数目
	char *sql_str = NULL;   //sql语句
  
	//设置字符编码为utf8
	mysql_setUTF8(dbconn);
	//设置插入语句
	sql_str = (char *)malloc(sizeof(char) * 200);
	memset(sql_str,0,200);
	sprintf(sql_str,"update ServerInfo set ip = %lu,port = %hu where account = '%s'", \
	         ip,port,account);
	//执行插入并判断插入是否成功
	int ret_query = mysql_query(dbconn->db_link,sql_str);
	affected_rows = mysql_affected_rows(dbconn->db_link); 
	if(ret_query)
	{
		debug("update private server error : %s",mysql_error(dbconn->db_link));
		recycleConn(dbconn);
		free(sql_str);
		return -1;
	}
   
	//更新成功     
	recycleConn(dbconn);
	free(sql_str);
	return 0;
}

