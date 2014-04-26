

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

insert into UserInfo(username,password) values("001","001");
insert into UserInfo(username,password) values("002","002");
insert into UserInfo(username,password) values("003","003");
insert into UserInfo(username,password) values("004","004");
insert into UserInfo(username,password) values("005","005");
insert into UserInfo(username,password) values("006","006");
insert into UserInfo(username,password) values("007","007");
insert into UserInfo(username,password) values("008","008");
insert into UserInfo(username,password) values("0719","0719");
insert into UserInfo(username,password) values("727","727");
insert into UserInfo(username,password) values("hebing","vincent");
insert into UserInfo(username,password) values("moyang","moyang");
insert into UserInfo(username,password) values("cct","cct");

insert into UserInServer(username,account) values("001","oil001");
insert into UserInServer(username,account) values("002","oil001");
insert into UserInServer(username,account) values("003","oil001");
insert into UserInServer(username,account) values("004","oil001");
insert into UserInServer(username,account) values("005","oil001");
insert into UserInServer(username,account) values("006","oil001");
insert into UserInServer(username,account) values("007","oil001");
insert into UserInServer(username,account) values("008","oil001");
insert into UserInServer(username,account) values("0719","oil001");
insert into UserInServer(username,account) values("727","oil001");
insert into UserInServer(username,account) values("vincent","oil001");
insert into UserInServer(username,account) values("moyang","oil001");
insert into UserInServer(username,account) values("cct","oil001");

