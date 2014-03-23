
#代理服务器程序makefile文件

#目标文件集
objects=agent_entry.o agent_threadtask.o agent_dbio.o agent_dbpool.o agent_sendlist.o agent_activeuser.o agent_sockio.o
args=-lpthread `mysql_config --cflags --libs` -std=c99

OilAgentServer : $(objects)
	gcc -g -o OilAgentServer $(objects) $(args)

agent_entry.o : agent_entry.c
	gcc -c agent_entry.c $(args)
agent_threadtask.o : agent_threadtask.h
	gcc -c agent_threadtask.c $(args)
agent_dbio.o : agent_dbio.h
	gcc -c agent_dbio.c $(args)
agent_dbpool.o : agent_dbpool.h
	gcc -c agent_dbpool.c $(args)
agent_sendlist.o : agent_sendlist.h
	gcc -c agent_sendlist.c $(args)
agent_activeuser.o : agent_activeuser.h
	gcc -c agent_activeuser.c $(args)
agent_sockio.o : agent_sockio.h
	gcc -c agent_sockio.c $(args)


#清除生成文件
.PHONY clean:
	-rm $(objects) OilAgentServer
