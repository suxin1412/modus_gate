
#ifndef _OPCUASERVER_H_
#define _OPCUASERVER_H_

// #define MECHANICAL_ARM //机械臂控制方法，宏开启

#include <stdint.h>
#include "open62541.h"

UA_Server *server;
UA_ServerConfig *config;
UA_Boolean running;

void UAServer_Start(char *conf);
void UAServer_Shutdown();
//void UAServer_RefreshDataDouble(int nameSpaceIndex,char *nodeIDString,int nodeIDNumeric,double data);

#endif
