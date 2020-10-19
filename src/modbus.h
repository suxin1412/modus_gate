#ifndef _MODBUS_H_
#define _MODBUS_H_

#include <stdint.h>

struct ModbusInfoStruct
{
	uint8_t slaveAddress;						//从机地址：0广播，1~47为单独的地址
	uint8_t functionCode;						//功能码
	uint16_t registerAddress;					//寄存器地址
	uint8_t baseDataBitSize;					//数据位长度，8,16,32
	double mappingCoefficient;					//映射比例
	uint8_t dataType;							//要转换的数据类型
};

int Modbus_Init(char *devAddr);
void Modbus_Shutdown();
void Modbus_PollFunc(void *protocolInfo, void *data);
void MechanicalArm_SendCMD(char *buf);
#endif
