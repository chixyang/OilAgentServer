
#include "agent_debug.h"

pthread_mutex_t log_write_lock = PTHREAD_MUTEX_INITIALIZER;

const char * log_file = "/var/log/oilagent.log";

int log_fd = -1;

char *log_buf = NULL;

//初始化日志文件
int initLog()
{
	//打开日志文件
	log_fd = open(log_file,O_WRONLY | O_APPEND);
	if(log_fd < 0)
	{
		perror("open log file error");
		return -1;
	}
	//分配日志缓存
	log_buf = (char *)malloc(log_buf_len * sizeof(char));
	if(log_buf == NULL)
	{
		perror("malloc log buf error");
		return -1;
	}

	return log_fd;
}

//获取当前时间字符串
char *NowTimeString()
{
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	return asctime(timenow);
}

