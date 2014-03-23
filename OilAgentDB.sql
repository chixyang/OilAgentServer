

create database OilAgentDB default character set 'utf8' collate 'utf8_general_ci';

use OilAgentDB;


create table ServerInfo
(
	account varchar(20) not null primary key,  
	ip		int unsigned,					
	port	smallint unsigned					
)engine=InnoDB default charset=utf8;



create table UserInfo
(
	username varchar(20) not null primary key, 
	password varchar(20) not null 
)engine=InnoDB default charset=utf8;



create table UserInServer
(
	username varchar(20) not null,
	account	varchar(20) not null,
	primary key (username,account),
	foreign key (username) references UserInfo(username),
	foreign key (account) references ServerInfo(account)
)engine=InnoDB default charset=utf8;


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
