#include <sys/types.h>		//系统常用类型定义
#include <sys/stat.h>		//系统常用文件状态定义
#include <fcntl.h>			//文件控制相关
#include <unistd.h>			//POSIX常用系统调用
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>		//POSIX线程管理
#include <string.h>
#include "OPCUAServer.h"	//OPC UA
#include "Polling.h"
#include <mysql.h>
MYSQL *con;
MYSQL_RES *MySQLRes;
MYSQL_ROW  MySQLRow;

pthread_t pollingThread;

void ExitGateway(int sig)
{
	void *p;
	UAServer_Shutdown();
	Prunning_running = 0;
	pthread_join(pollingThread, &p);
	usleep(1000);
	exit(0);
}


int main(int argc,char *args[])
{
	pid_t fpid = fork();
	if (fpid<0) {printf("ERROR 005: Fork Error.");return -1;}
	if (fpid==0) {
		//以下数据库操作
		printf("SQL start...");
		fflush(stdout);
		con = mysql_init(NULL);																	//初始化
		if (!mysql_real_connect(con, "127.0.0.1", "root", "raspberry1412", "Gateway", 0, NULL, 0))//连接
		{
			printf("%s\n", mysql_error(con));
			mysql_close(con);
			printf("SQL connect Error.");
			fflush(stdout);
			exit(EXIT_FAILURE);
		}
		// int sqlERR = mysql_query(con, "SELECT * FROM configFiles");								//执行语句
		// MySQLRes = mysql_store_result(con);														//获取返回的结果，多行
		// while (MySQLRow = mysql_fetch_row(MySQLRes)){											//从结果中取出一行
		// 	unsigned int count=0;
		// 	while(count<mysql_field_count(con)){
		// 		printf("\t%s",(MySQLRow)[count]);
		// 		count++;
		// 	}
		// 	printf("\n");
		// }
		// printf("SQL:%d, row%lu\n",sqlERR,(unsigned long)mysql_num_rows(MySQLRes));
		mysql_query(con, "UPDATE GatewayInfo SET info_value = NOW() WHERE info_key = 'LaunchTime';");								//执行语句
		char SQLBuf[100];
		sprintf(SQLBuf,"UPDATE GatewayInfo SET info_value = '%d' WHERE info_key = 'PID';",fpid);
		mysql_query(con, SQLBuf);								//执行语句
		mysql_close(con);
		printf("SQL Process Over.");
		fflush(stdout);
		return 0;
	}
	// printf("child PID :%d\n",fpid);
	if (argc<2) {printf("ERROR 001: Config file not specified.");return -1;}				//未传入参数：配置文件路径-直接退出返回
	int fd_conf = 0;  																		//读取配置文件，获取文件内容
	fd_conf = open(args[1],O_RDONLY|O_NDELAY);
	if (fd_conf<0) {printf("ERROR 002: Config file can not open.");return -1;}
	struct stat *statbuf = (struct stat *)malloc(sizeof(struct stat)); 
	if (statbuf==NULL) {printf("ERROR 003: Insufficient memory.");return -1;}
	stat(args[1],statbuf); 																//获取文件大小，用于将文件内容全部读出
	char *p,*pcp;																			//存放文件内容的指针
	p = (char *)malloc(statbuf->st_size);
	pcp = (char *)malloc(statbuf->st_size);
	if (p==NULL) {printf("ERROR 003: Insufficient memory.");return -1;}
	int readcount = 0;
	readcount = read(fd_conf,p,statbuf->st_size);											//读取文件内容
	if (readcount!=statbuf->st_size) {printf("ERROR 004: Read config file error.%d,,,%d\n",readcount,statbuf->st_size);return -1;}
	memcpy(pcp,p,statbuf->st_size);															//内存复制：将配置复制成两份
	free(statbuf);
	close(fd_conf);																			//配置文件get，为两个线程准备的两份数据ready
	///需要处理中断信号
	signal(SIGINT,ExitGateway);
	pthread_create(&pollingThread, NULL, Polling_Start, (void*)pcp);
	sleep(1);
	printf("Create Poll thread!\n");
	UAServer_Start(p);																		//OPCUA服务器初始化，指针p不需要释放，函数内部释放
	return 0;
}













