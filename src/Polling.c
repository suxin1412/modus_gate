#include "Polling.h"
#include "OPCUAServer.h"
#include "cJSON.h"
#include <string.h>
#include <pthread.h>
#include <mysql.h>

#ifdef MECHANICAL_ARM
uint8_t ArmCMDFlag = 0;
char ArmCMDBuffer[15];
#endif


int Polling_CreateTaskListFromConfiguration(char *p);
int Polling_CreateTaskNodeFromConfiguration(cJSON *subArrayRoot, int index);

/********************************************************
	【函数名称】 Polling_Start
	【函数功能】 启动设备层数据轮询线程
	【参数】 
		arg 配置文件指针
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
	【更改记录】
********************************************************/
void *Polling_Start(void *arg)
{
	Prunning_running = 1;
	double i = 0;
//	Polling_TaskList = (struct Polling_TaskList_Struct*)malloc(sizeof(struct Polling_TaskList_Struct));//头结点
//	Polling_TaskList->child = NULL;
	//memset(Polling_TaskList, 0, sizeof(struct Polling_TaskList_Struct*)*100);
	for (scanfPointer=0;scanfPointer<100;scanfPointer++)
		Polling_TaskList[scanfPointer] = NULL;
	scanfPointer = 0;
	Polling_CreateTaskListFromConfiguration((char*)arg);
	scanfPointer = 0;
	printf("Creat Task List........ok\n");
	Modbus_Init("/dev/ttyUSB0");
	printf("Open ttyS1........ok\n");
	for (;scanfPointer<=Polling_TaskListLength;scanfPointer++){
		printf("Polling Queue\t index:%d, RQ:%d\n",scanfPointer,Polling_TaskList[scanfPointer]->refreshFrequency);
	}
	scanfPointer = 0;
	printf("Polling_TaskListLength:%d\n",Polling_TaskListLength);
	while (Prunning_running)
	{
		//UAServer_RefreshDataDouble(1, "Temperature", 0, i);
		Polling_TaskList[scanfPointer]->Function(Polling_TaskList[scanfPointer]->protocolInfo,Polling_TaskList[scanfPointer]->data);
		// printf("@@Read Reg :: registerAddress:%u, scanfPointer:%d, data:%f\n",
		// 			((struct ModbusInfoStruct*)(Polling_TaskList[scanfPointer]->protocolInfo))->registerAddress,
		// 			scanfPointer,
		// 			*((double*)Polling_TaskList[scanfPointer]->data));
		fflush(stdout);
		scanfPointer++;
		if (Polling_TaskList[scanfPointer]==NULL) {
			scanfPointer = 0;
			#ifdef MECHANICAL_ARM
			if (ArmCMDFlag){
				MechanicalArm_SendCMD(ArmCMDBuffer);
			}
			#endif
		}
		i += 1.35;
		//usleep(50000);
	}
	Modbus_Shutdown();
	pthread_exit("bye");
	return NULL;
}

/********************************************************
	【函数名称】 Polling_CreateTaskListFromConfiguration
	【函数功能】 从配置文件创建任务列表
	【参数】 
		p 配置文件指针
	【返回值】 
		0  正确配置
		<0 错误
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
	【更改记录】
********************************************************/
int Polling_CreateTaskListFromConfiguration(char *p)
{
	cJSON *root = cJSON_Parse(p); 											//将Json串解析成为cJSON格式
	//printf(p);
	free(p);																//释放文件内容指针
	if (cJSON_HasObjectItem(root,"AddressSpace") == 0) return -1;			//判断配置文件是否合法：存在“AddressSpace”字段
	cJSON *addressSpace = cJSON_GetObjectItem(root,"AddressSpace");			//获取上述字段对象（数组）
	int nodeCount = cJSON_GetArraySize(addressSpace); //下标从0开始				//统计数组元素个数
	if (nodeCount <= 0) return -2;											//字段错误或无元素
	for (int i=0;i<nodeCount;i++){												//循环:检查每个元素
		int err = Polling_CreateTaskNodeFromConfiguration(addressSpace, i); //
		if (err!=0) return err;
	}
	cJSON_Delete(root);														//删除JSON指针，释放空间
	return 0;
}

/********************************************************
	【函数名称】 Polling_CreateTaskNodeFromConfiguration
	【函数功能】 从配置文件创建任务节点插入任务列表
	【参数】 
		subArrayRoot 子节点数组的json字符串
		index 要分析的节点在数组中的下标
	【返回值】 
		0  正确配置
		<0 错误
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
	【更改记录】
********************************************************/
int Polling_CreateTaskNodeFromConfiguration(cJSON *subArrayRoot, int index)
{
	cJSON *node = cJSON_GetArrayItem(subArrayRoot, index);					//取节点
	printf("Analyse Node :: Index: %d \tType: %s \tname:%s\n",index,cJSON_GetObjectItem(node,"NodeClass")->valuestring,cJSON_GetObjectItem(node,"BrowseName")->valuestring);
	if (cJSON_HasObjectItem(node,"BottomProtocolType"))					//判断有没有数据
	{
		Polling_TaskList[scanfPointer] = (struct Polling_TaskList_Struct*)malloc(sizeof(struct Polling_TaskList_Struct));//生成表项
		Polling_TaskListLength = scanfPointer;
		printf("\tTaskList Node  :: Index: %d \tAddr:%x\n",scanfPointer,Polling_TaskList[scanfPointer]);
		printf("\t\tNode Info  :: refreshFrequency: %d \tMapRadio:%f\n",cJSON_GetObjectItem(node,"RefreshFrequency")->valueint,cJSON_GetObjectItem(node,"MappingCoefficient")->valuedouble);
		//scanfPointer = scanfPointer->child;																	//指针向后移动
		//scanfPointer->child = NULL;
		Polling_TaskList[scanfPointer]->refreshFrequency = cJSON_GetObjectItem(node,"RefreshFrequency")->valueint;			//更新频率
		Polling_TaskList[scanfPointer]->counter = Polling_TaskList[scanfPointer]->refreshFrequency;												//计数器，向下生长
		if (strcmp(cJSON_GetObjectItem(node,"BottomProtocolType")->valuestring,"Modbus")==0)				//判断协议为modbus
		{
			Polling_TaskList[scanfPointer]->bottomProtocolType = MODBUS;														//协议类型赋值
			Polling_TaskList[scanfPointer]->protocolInfo = malloc(sizeof(struct Polling_TaskList_Struct));					//为协议配置信息申请空间
			((struct ModbusInfoStruct *)Polling_TaskList[scanfPointer]->protocolInfo)->slaveAddress = cJSON_GetObjectItem(node,"MB_MachineAddress")->valueint;//从机地址
			((struct ModbusInfoStruct *)Polling_TaskList[scanfPointer]->protocolInfo)->functionCode = cJSON_GetObjectItem(node,"MB_FunctionCode")->valueint;//功能吗
			((struct ModbusInfoStruct *)Polling_TaskList[scanfPointer]->protocolInfo)->registerAddress = cJSON_GetObjectItem(node,"MB_RegisterAddress")->valueint;
			((struct ModbusInfoStruct *)Polling_TaskList[scanfPointer]->protocolInfo)->baseDataBitSize = cJSON_GetObjectItem(node,"MB_DataBitSize")->valueint;
			((struct ModbusInfoStruct *)Polling_TaskList[scanfPointer]->protocolInfo)->mappingCoefficient = cJSON_GetObjectItem(node,"MappingCoefficient")->valuedouble;
			((struct ModbusInfoStruct *)Polling_TaskList[scanfPointer]->protocolInfo)->dataType = 
								strcmp(cJSON_GetObjectItem(node,"DataType")->valuestring,"Double")==0?0:(
								strcmp(cJSON_GetObjectItem(node,"DataType")->valuestring,"Uint8_t")==0?1:(2)
								);///其他数据类型########
			Polling_TaskList[scanfPointer]->Function = Modbus_PollFunc;//用于处理modbus协议的函数指针
			switch(((struct ModbusInfoStruct*)(Polling_TaskList[scanfPointer]->protocolInfo))->dataType)
			{
				case 0:
					Polling_TaskList[scanfPointer]->data = malloc(sizeof(double));
					break;
				case 1:
					Polling_TaskList[scanfPointer]->data = malloc(sizeof(uint8_t));
					break;
				case 2:
					Polling_TaskList[scanfPointer]->data = malloc(sizeof(uint16_t));
					break;
				case 3:
					Polling_TaskList[scanfPointer]->data = malloc(sizeof(uint32_t));
					break;
				case 4:
					Polling_TaskList[scanfPointer]->data = malloc(sizeof(int8_t));
					break;
				case 5:
					Polling_TaskList[scanfPointer]->data = malloc(sizeof(int16_t));
					break;
				case 6:
					Polling_TaskList[scanfPointer]->data = malloc(sizeof(int32_t));
					break;
				default:
					return -1;
					break;
			}
		}else ;///其他协议########
		++scanfPointer;
	}
	if (cJSON_HasObjectItem(node,"ChildNode")){																//检测是否有“ChildNode”字段
		int nodeCount = 0,i = 0;
		nodeCount = cJSON_GetArraySize(cJSON_GetObjectItem(node,"ChildNode"));
		for (i=0;i<nodeCount;i++){																			//循环：为每个元素生成节点
			int err = Polling_CreateTaskNodeFromConfiguration(cJSON_GetObjectItem(node,"ChildNode"), i);	//嵌套
			if (err!=0) return err;
		}	
	}
	return 0;
}








