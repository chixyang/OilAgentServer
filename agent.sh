#!/bin/sh

proc_name="OilAgentServer"                             # 进程名
file_name="/root/runner/debug.log"    # 日志文件
pid=0

proc_num()                                              # 计算进程数
{
	num=`ps -ef | grep $proc_name | grep -v grep | wc -l`
	return $num
}

proc_id()                                               # 进程号
{
	pid=`ps -ef | grep $proc_name | grep -v grep | awk '{print $2}'`
}

proc_num
number=$?
if [ $number -eq 0 ]                                    # 判断进程是否存在
then 
	cd /root/runner/;  `./OilAgentServer`          #或者exec
	proc_id                                         # 获取新进程号
	echo ${pid}, `date` >> $file_name      # 将新进程号和重启时间记录
fi
