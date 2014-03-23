/**
 * 此文件用于debug宏的定义
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <time.h>
#include <stdio.h>


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
#define MAXDATALENGTH 512      //单次最大协议长度不超过512字节

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef uint32 ip_t;


#define DEBUG


#ifdef DEBUG
#define debug(...) do{                                                      		\
	           printf(RED"debug info: ***");                                     	\
                   printf(__VA_ARGS__);                                     		\
		   time_t now;								\
		   struct tm *timenow;							\
		   time(&now);								\
		   timenow = localtime(&now);						\
	           printf(GREEN"***debug line:%d,debug function:%s ,debug file:%s ,debug time:%s\n"NONE,__LINE__,__FUNCTION__,__FILE__,asctime(timenow)); 	\
                   }while(0)
#else 
#define debug(...) do{}while(0)
#endif











#endif
