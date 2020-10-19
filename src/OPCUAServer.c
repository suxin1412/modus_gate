#include "OPCUAServer.h"
#include "cJSON.h"
#include <string.h>
#include "Polling.h"
#include <stdio.h>
#include <stdlib.h>


#ifdef MECHANICAL_ARM
static UA_StatusCode UAServer_ArmCtrlMethodCallback(UA_Server *server,
                         			const UA_NodeId *sessionId, void *sessionHandle,
                         			const UA_NodeId *methodId, void *methodContext,
                         			const UA_NodeId *objectId, void *objectContext,
                         			size_t inputSize, const UA_Variant *input,
                         			size_t outputSize, UA_Variant *output);
static void addMethod(UA_Server *server, void *func);
#endif

//void UAServer_AddModelToServer(UA_Server *server);							//旧模型构造函数
int UAServer_BuildModelFromConfiguration(char *conf);									 //从配置文件生成模型
int UAServer_AddNodeFromConfiguration(cJSON *subArrayRoot, int index, UA_NodeId parent); //向模型中添加单个节点
// struct UAServer_NodeIDMapToIndexType{
// UA_NodeId nodeid
// int index;
// };
// struct UAServer_NodeIDMapToIndexType *UAServer_NodeIDMapToIndexArray[100];
int UAServer_NodeIDHashMapToIndexArrayPos = 0;
uint32_t UAServer_NodeIDHashMapToIndexArray[100];

void UAServer_RefreshDataDouble(const UA_NodeId *nodeid, double data); //变量更新函数
static void UAServer_BeforeReadCallBack(UA_Server *server,
										const UA_NodeId *sessionId, void *sessionContext,
										const UA_NodeId *nodeid, void *nodeContext,
										const UA_NumericRange *range, const UA_DataValue *data); //回调函数
static void UAServer_AfterWriteCallBack(UA_Server *server,
										const UA_NodeId *sessionId, void *sessionContext,
										const UA_NodeId *nodeId, void *nodeContext,
										const UA_NumericRange *range, const UA_DataValue *data);
static void UAServer_AddVariableCallback(UA_NodeId currentNodeId);

/********************************************************
	【函数名称】 UAServer_Start
	【函数功能】 启动UA服务器
	【参数】 
		conf 配置文件指针
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
	【更改记录】
********************************************************/
void UAServer_Start(char *conf)
{
	//memset(UAServer_NodeIDMapToIndexArray, 0, sizeof(UA_NodeId *)*100);
	//config = UA_ServerConfig_new_default();

	server = UA_Server_new();
	printf("UA Server is Running! 1");
	fflush(stdout);
	//UA_ServerConfig_setDefault(UA_Server_getConfig(server));
	UA_ServerConfig_setMinimal(UA_Server_getConfig(server), 14523, NULL);
	printf("UA Server is Running! 2");
	fflush(stdout);
	running = true;
	//UAServer_AddModelToServer(server);
	//free(conf);
	int err = UAServer_BuildModelFromConfiguration(conf);
	printf("UA Server is Running! 3");
	fflush(stdout);
	//printf("New NodeID's hash are %d and %d, totle:%d.\n", UA_NodeId_hash(UAServer_NodeIDMapToIndexArray[0]),UA_NodeId_hash(UAServer_NodeIDMapToIndexArray[1]),UAServer_NodeIDMapToIndexArrayPos);
	//printf(UAServer_NodeIDMapToIndexArray[0]->identifier);
	// curServerConfig = UA_Server_getConfig(server);
	UA_StatusCode status = UA_Server_run(server, &running);
	printf("UA Server is Running! 4");

	UA_Server_delete(server);
	//UA_ServerConfig_delete(config);
}

/********************************************************
	【函数名称】 UAServer_Shutdown
	【函数功能】 关闭UA服务器
	【参数】 无
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
void UAServer_Shutdown()
{
	running = false;
}

/********************************************************
	【函数名称】 UAServer_BuildModelFromConfiguration
	【函数功能】 从配置文件创建地址空间模型
	【参数】 
		conf 配置文件指针
	【返回值】
		0 创建成功
		other 失败
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
	【更改记录】
********************************************************/
int UAServer_BuildModelFromConfiguration(char *conf)
{
	int nodeCount = 0, i = 0;
	cJSON *root = cJSON_Parse(conf); //将Json串解析成为cJSON格式
	free(conf);						 //释放文件内容指针
	if (cJSON_HasObjectItem(root, "AddressSpace") == 0)
		return -1;													 //判断配置文件是否合法：存在“AddressSpace”字段
	cJSON *addressSpace = cJSON_GetObjectItem(root, "AddressSpace"); //获取上述字段对象（数组）
	nodeCount = cJSON_GetArraySize(addressSpace);					 //下标从0开始				//统计数组元素个数
	if (nodeCount <= 0)
		return -2; //字段错误或无元素
	for (i = 0; i < nodeCount; i++)
	{ //循环：为每个元素生成节点
		int err = UAServer_AddNodeFromConfiguration(addressSpace, i, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER));
		if (err != 0)
			return err;
	}
	#ifdef MECHANICAL_ARM
	addMethod(server, UAServer_ArmCtrlMethodCallback);
	#endif
	cJSON_Delete(root); //删除JSON指针，释放空间
	return 0;
}

/********************************************************
	【函数名称】 UAServer_AddNodeFromConfiguration
	【函数功能】 从配置文件创建地址空间结点
	【参数】 
		subArrayRoot 子节点数组json字符串
		index 要创建的节点在数组中的下标
		parent 父节点ID
	【返回值】 】
		0 创建成功
		other 失败
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
	【更改记录】
********************************************************/
int UAServer_AddNodeFromConfiguration(cJSON *subArrayRoot, int index, UA_NodeId parent)
{
	cJSON *node = cJSON_GetArrayItem(subArrayRoot, index); //获取元素指针:数组的index项
	//printf("%s",cJSON_Print(cJSON_GetObjectItem(node,"NodeID_IDType")));
	UA_NodeId thisNodeID; //构造node的成员：nodeID
	if (strcmp(cJSON_GetObjectItem(node, "NodeID_IDType")->valuestring, "String") == 0)
	{																																					  //判断节点ID类型：字符串
		thisNodeID = UA_NODEID_STRING(cJSON_GetObjectItem(node, "NodeID_NamespaceIndex")->valueint, cJSON_GetObjectItem(node, "NodeID_ID")->valuestring); //按照字符串生成节点ID
	}
	else if (strcmp(cJSON_GetObjectItem(node, "NodeID_IDType")->valuestring, "Numeric") == 0)															//判断节点ID类型：数字
		thisNodeID = UA_NODEID_NUMERIC(cJSON_GetObjectItem(node, "NodeID_NamespaceIndex")->valueint, cJSON_GetObjectItem(node, "NodeID_ID")->valueint); //按照Numeric生成节点ID
	UA_NodeId refNodeID = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);																						//获取引用类型
	if (strcmp(cJSON_GetObjectItem(node, "ReferenceTypeId")->valuestring, "Organizes") == 0)															//引用类型：组织（在文件夹）
		refNodeID = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	else if (strcmp(cJSON_GetObjectItem(node, "ReferenceTypeId")->valuestring, "HsaComponent") == 0) //引用类型：组件
		refNodeID = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
	else if (strcmp(cJSON_GetObjectItem(node, "ReferenceTypeId")->valuestring, "HasProperty") == 0) //引用类型：属性
		refNodeID = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
	if (strcmp(cJSON_GetObjectItem(node, "NodeClass")->valuestring, "Object") == 0)
	{																											//判断节点类型:obj
		UA_ObjectAttributes attrObj = UA_ObjectAttributes_default;												//构造Node的成员：节点属性
		attrObj.description = UA_LOCALIZEDTEXT("en-US", cJSON_GetObjectItem(node, "Description")->valuestring); //属性：描述
		attrObj.displayName = UA_LOCALIZEDTEXT("en-US", cJSON_GetObjectItem(node, "DisplayName")->valuestring);
		UA_Server_addObjectNode(server, //添加到地址空间
								thisNodeID,
								parent,
								refNodeID,
								UA_QUALIFIEDNAME(1, cJSON_GetObjectItem(node, "QualifiedName")->valuestring),
								UA_NODEID_NULL,
								attrObj, NULL, NULL);
	}
	else if (strcmp(cJSON_GetObjectItem(node, "NodeClass")->valuestring, "Variable") == 0)
	{																											//判断节点类型：容器
		UA_VariableAttributes attrVar = UA_VariableAttributes_default;											//构造Node的成员：容器节点属性
		attrVar.description = UA_LOCALIZEDTEXT("en-US", cJSON_GetObjectItem(node, "Description")->valuestring); //属性：描述
		attrVar.displayName = UA_LOCALIZEDTEXT("en-US", cJSON_GetObjectItem(node, "DisplayName")->valuestring);
		if (strcmp(cJSON_GetObjectItem(node, "DataType")->valuestring, "Double") == 0)
		{ //判断容器数据类型：double
			UA_Double *temp = (UA_Double *)malloc(sizeof(UA_Double));
			attrVar.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_DOUBLE);
			*temp = cJSON_GetObjectItem(node, "InitialValue")->valuedouble;
			//printf("------------------------%x\n",cJSON_GetObjectItem(node,"InitialValue")->type);
			UA_Variant_setScalar(&attrVar.value, temp, &UA_TYPES[UA_TYPES_DOUBLE]);
		}
		else
			return -4; ///@@@@@@@@@@@@@@@@@@不同的数据类型
		UA_Server_addVariableNode(server,
								  thisNodeID,
								  parent,
								  UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
								  UA_QUALIFIEDNAME(1, cJSON_GetObjectItem(node, "QualifiedName")->valuestring),
								  UA_NODEID_NULL,
								  attrVar, NULL, NULL);
		printf("Has add NodeID %s, it's hash is %d, index is %d.\n", cJSON_GetObjectItem(node, "DisplayName")->valuestring, UA_NodeId_hash(&thisNodeID), UAServer_NodeIDHashMapToIndexArrayPos);
		UAServer_NodeIDHashMapToIndexArray[UAServer_NodeIDHashMapToIndexArrayPos++] = UA_NodeId_hash(&thisNodeID);
		if (UAServer_NodeIDHashMapToIndexArrayPos == 100)
			UAServer_NodeIDHashMapToIndexArrayPos = 99;
		UAServer_AddVariableCallback(thisNodeID);
	}
	else
		return -3; ///####################不同的节点类型
	if (cJSON_HasObjectItem(node, "ChildNode"))
	{ //检测是否有“ChildNode”字段
		int nodeCount = 0, i = 0;
		nodeCount = cJSON_GetArraySize(cJSON_GetObjectItem(node, "ChildNode"));
		for (i = 0; i < nodeCount; i++)
		{ //循环：为每个元素生成节点
			int err = UAServer_AddNodeFromConfiguration(cJSON_GetObjectItem(node, "ChildNode"), i, thisNodeID);
			if (err != 0)
				return err;
		}
	}
	return 0;
}

/*	UAServer_RefreshDataDouble 地址空间中的数据更新函数
 *	若节点ID是字符串类型则使用nodeIDString传入，否则需要将nodeIDString置为NULL，使用nodeIDNumeric传入。
 */
/*void UAServer_RefreshDataDouble(int nameSpaceIndex,char *nodeIDString,int nodeIDNumeric,double data)
{
	UA_NodeId nodeID;
	if (nodeIDString==NULL)
		nodeID = UA_NODEID_NUMERIC(nameSpaceIndex, nodeIDNumeric);
	else 
		nodeID = UA_NODEID_STRING(nameSpaceIndex, nodeIDString);
	UA_Variant var ;
	UA_Variant_setScalar(&var,&data,&UA_TYPES[UA_TYPES_BOOLEAN]);
	UA_Server_writeValue(server,nodeID,var);
}*/
/********************************************************
	【函数名称】 UAServer_RefreshDataDouble
	【函数功能】 将数据刷新到结点中
	【参数】 
		nodeid 要刷新数据的节点ID
		data 要刷新的数据
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
void UAServer_RefreshDataDouble(const UA_NodeId *nodeid, double data)
{
	UA_Variant var;
	UA_Variant_setScalar(&var, &data, &UA_TYPES[UA_TYPES_DOUBLE]);
	UA_Server_writeValue(server, *nodeid, var);
}

/*
 * 函数:UAServer_BeforeReadCallBack 客户端读取前的回调函数
 * 
 */
 /********************************************************
	【函数名称】 UAServer_UAStringToChar
	【函数功能】 将UA_String字符串转换为char*
	【参数】 str 要转换的字符串
	【返回值】 转换后的字符串指针 
	【注意】 需要调用后由调用者释放
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
char *UAServer_UAStringToChar(UA_String str)
{
	char *p = (char *)malloc(sizeof(char) * (str.length + 1));
	int i = 0;
	for (; i < str.length; i++)
	{
		p[i] = str.data[i];
	}
	p[i] = (char)0;
	return p;
}

 /********************************************************
	【函数名称】 UAServer_BeforeReadCallBack
	【函数功能】 UA客户端读取数据前的回调函数
	【参数】 
		server UA服务器指针
		nodeid 要读取的结点的ID
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
static void UAServer_BeforeReadCallBack(UA_Server *server,
										const UA_NodeId *sessionId, void *sessionContext,
										const UA_NodeId *nodeid, void *nodeContext,
										const UA_NumericRange *range, const UA_DataValue *data)
{
	char buf[50];
	static double tmp = 1.373;
	// tmp += 0.377337;
	int i;
	for (i = 0; i < UAServer_NodeIDHashMapToIndexArrayPos; i++)
		if (UA_NodeId_hash(nodeid) == UAServer_NodeIDHashMapToIndexArray[i])
			break;
	if (i == UAServer_NodeIDHashMapToIndexArrayPos)
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ERROR: can't find map.");
		return;
	}
	tmp = *((double *)Polling_TaskList[i]->data);
	sprintf(buf, "This Node's hash is %d, index is %d", UAServer_NodeIDHashMapToIndexArray[i], i);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, buf);
	//printf("Session NodeID's Type:%x, Name:%s\n", curServerConfig->customHostname.length, UAServer_UAStringToChar(curServerConfig->serverCertificate));
	UAServer_RefreshDataDouble(nodeid, tmp);
}

 /********************************************************
	【函数名称】 UAServer_AfterWriteCallBack
	【函数功能】 UA客户端读取数据之后的回调函数
	【参数】 
		server UA服务器指针
		nodeid 要读取的结点的ID
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
static void UAServer_AfterWriteCallBack(UA_Server *server,
										const UA_NodeId *sessionId, void *sessionContext,
										const UA_NodeId *nodeId, void *nodeContext,
										const UA_NumericRange *range, const UA_DataValue *data)
{
	return;
	//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "The variable was updated");
}
 /********************************************************
	【函数名称】 UAServer_AddVariableCallback
	【函数功能】 为UA服务器添加变量节点的回调函数
	【参数】 
		currentNodeId 要添加回掉函数的结点ID
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
static void UAServer_AddVariableCallback(UA_NodeId currentNodeId)
{
	UA_ValueCallback callback;
	callback.onRead = UAServer_BeforeReadCallBack;
	callback.onWrite = UAServer_AfterWriteCallBack;
	UA_Server_setVariableNode_valueCallback(server, currentNodeId, callback);
}

/********************************************************
 *	为机械臂模型添加方法
 ********************************************************/
#ifdef MECHANICAL_ARM
 /********************************************************
	【函数名称】 UAServer_AddVariableCallback
	【函数功能】 为UA服务器添加变量节点的回调函数
	【参数】 
		currentNodeId 要添加回掉函数的结点ID
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
static UA_StatusCode UAServer_ArmCtrlMethodCallback(UA_Server *server,
                         			const UA_NodeId *sessionId, void *sessionHandle,
                         			const UA_NodeId *methodId, void *methodContext,
                         			const UA_NodeId *objectId, void *objectContext,
                         			size_t inputSize, const UA_Variant *input,
                         			size_t outputSize, UA_Variant *output) 
{
    // UA_String *inputStr = (UA_String*)input->data;
    // UA_String tmp = UA_STRING_ALLOC("Hello ");
    // if(inputStr->length > 0) {
    //     tmp.data = (UA_Byte *)UA_realloc(tmp.data, tmp.length + inputStr->length);
    //     memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
    //     tmp.length += inputStr->length;
    // }
    // UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    // UA_String_clear(&tmp);
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
	uint8_t SteeringEngineIndex = *(uint8_t*)input[0].data;
	float SteeringEngineAimAngle = *(float*)input[1].data;
	uint16_t SteeringEngineCtrlTime = *(uint16_t*)input[2].data;
	if (SteeringEngineAimAngle>80.0||SteeringEngineAimAngle<-80.0||SteeringEngineIndex>5||SteeringEngineCtrlTime>9999){
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "参数错误!");
    	return UA_STATUSCODE_BADINVALIDARGUMENT;
	}
	if (SteeringEngineAimAngle>=0)
		sprintf(ArmCMDBuffer, "$%1d+%04.1f%04d!", SteeringEngineIndex, SteeringEngineAimAngle, SteeringEngineCtrlTime);
	else 
		sprintf(ArmCMDBuffer, "$%1d-%04.1f%04d!", SteeringEngineIndex, -SteeringEngineAimAngle, SteeringEngineCtrlTime);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, ArmCMDBuffer);
	ArmCMDFlag = 1;
    return UA_STATUSCODE_GOOD;
}
static void addMethod(UA_Server *server, void *func)
{
    UA_Argument inputArgument[3];
    UA_Argument_init(inputArgument);
    inputArgument[0].description = UA_LOCALIZEDTEXT("en-US", "关节序号");
    inputArgument[0].name = UA_STRING("关节序号");
    inputArgument[0].dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
    inputArgument[0].valueRank = UA_VALUERANK_SCALAR;

	
    UA_Argument_init(inputArgument+1);
    inputArgument[1].description = UA_LOCALIZEDTEXT("en-US", "关节角度");
    inputArgument[1].name = UA_STRING("关节角度");
    inputArgument[1].dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    inputArgument[1].valueRank = UA_VALUERANK_SCALAR;
	
    UA_Argument_init(inputArgument+2);
    inputArgument[2].description = UA_LOCALIZEDTEXT("en-US", "控制时间");
    inputArgument[2].name = UA_STRING("控制时间");
    inputArgument[2].dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
    inputArgument[2].valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US","机械臂控制");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US","机械臂控制");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1,"机械臂控制"),
                            UA_NODEID_STRING(1, "机械臂"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "机械臂控制"),
                            helloAttr, func,
                            3, inputArgument, 0, &outputArgument, NULL, NULL);
}
#endif

