
--创建代理服务器数据库
create database OilAgentDB default character set 'utf8' collate 'utf8_general_ci';

use OilAgentDB;

--ServerInfo表
create table ServerInfo
(
	account varchar(20) not null primary key,  --机房服务器账户
	ip		int unsigned,					--机房服务器ip,可以为NULL，第一次录入时即为NULL
	port	smallint unsigned					--机房服务器端口
)engine=InnoDB default charset=utf8;


--UserInfo表
create table UserInfo
(
	username varchar(20) not null primary key, --用户名
	password varchar(20) not null --密码
)engine=InnoDB default charset=utf8;


--UserInServer表
create table UserInServer
(
	username varchar(20) not null,
	account	varchar(20) not null,
	primary key (username,account),
	foreign key (username) references UserInfo(username),
	foreign key (account) references ServerInfo(account)
)engine=InnoDB default charset=utf8;

--插入数据
use OilAgentDB;

insert into ServerInfo(account) values("oil001");
insert into ServerInfo(account) values("oil002");
insert into ServerInfo(account) values("oil003");
insert into ServerInfo(account) values("oil004");
insert into ServerInfo(account) values("oil005");
insert into ServerInfo(account) values("oil006");
insert into ServerInfo(account) values("oil007");
insert into ServerInfo(account) values("oil008");
insert into ServerInfo(account) values("oil009");
insert into ServerInfo(account) values("oil010");

insert into UserInfo(username,password) values("oil001admin","admin");
insert into UserInfo(username,password) values("oil001guest","12333");
insert into UserInfo(username,password) values("oil001user","34555");

insert into UserInServer(username,account) values("oil001admin","oil001");
insert into UserInServer(username,account) values("oil001guest","oil001");
insert into UserInServer(username,account) values("oil001user","oil001");