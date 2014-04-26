/**
 * 此文件用于debug宏的定义
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define NONE         "\033[m"  
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define DARY_GRAY    "\033[1;30m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define PURPLE       "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"


//最大协议数据长度
#define MAXDATALENGTH 512      //单次最大数据长度不超过512字节

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned long ip_t;

extern pthread_mutex_t log_write_lock;  //日志写锁
extern int log_fd;      //日志文件描述符
extern char *log_buf;   //日志缓存


#define log_buf_len 256

#define DEBUG

#ifdef DEBUG
#define debug(...) do{                                                      		\
		while(pthread_mutex_trylock(&log_write_lock));                                      		\
					memset(log_buf,0,log_buf_len);  \
                int log_w = sprintf(log_buf,__VA_ARGS__);    \
			log_w += sprintf(log_buf+log_w,"\nline:%d,function:%s,file:%s,time:%s\n",__LINE__,__FUNCTION__,__FILE__,NowTimeString());					\
				write(log_fd,log_buf,log_w);  \
			pthread_mutex_unlock(&log_write_lock);									\
					}while(0) 
#else 
#define debug(...) do{}while(0)
#endif


/**
 * 初始化日志文件
 * @return 日志文件描述符
 */
int initLog();

/**
 * 获取当前时间字符串
 * @return 当前时间字符串
 */
char *NowTimeString();




#endif
