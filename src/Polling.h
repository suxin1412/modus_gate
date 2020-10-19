#ifndef _POLLING_H_
#define _POLLING_H_

#include "modbus.h"
#include <stdint.h>

//底层协议类型定义 用于bottomProtocolType字段
#define MODBUS 		0

//转换后的数据类型定义 用于dataType字段
#define DOUBLE 		0
#define UINT8_T 	1
#define UINT16_T 	2
#define UINT32_T 	3
#define INT8_T 		4
#define INT16_T 	5
#define INT32_T 	6
#define STRING		7


struct Polling_TaskList_Struct
{
	int refreshFrequency;						//刷新频率
	int counter;								//计数器，向下生长
	int bottomProtocolType;						//底层协议类型
	void *protocolInfo;							//协议配置信息
	void *data;									//数据
	void (*Function)(void *protocolInfo, void *data);//数据采集函数
};

struct Polling_TaskList_Struct *Polling_TaskList[100];		//任务列表，带头结点的链表
//struct Polling_TaskList_Struct *scanfPointer;			//扫描指针，建表时指向表尾
int scanfPointer;			//扫描指针，建表时指向表尾
int Polling_TaskListLength;			//扫描指针，建表时指向表尾

#define MECHANICAL_ARM //机械臂控制方法，宏开启
#ifdef MECHANICAL_ARM
extern uint8_t ArmCMDFlag;
extern char ArmCMDBuffer[15];
#endif

void *Polling_Start(void *arg);
int Prunning_running;



#endif

