#!/bin/bash
                                      
proc_name="OilAgentServer"                             # 进程名
pid=0

proc_num()                                              # 计算进程数
{
	num=`ps -ef | grep $proc_name | grep -v grep | wc -l`
	return $num
}

proc_num
number=$?
if [ $number -eq 0 ]                                    # 判断进程是否存在
then 
	`OilAgentServer`;               #运行新进程
fi
