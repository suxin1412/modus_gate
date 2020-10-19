#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdint.h>
#include "modbus.h"


int fd;//串口的句柄
uint8_t buffer[20];
uint8_t pos = 0;

/*串口初始化函数
 *@method Usart_Init
 *@param{void}
 *@return {void}
*/
void Usart_Init()
{
	struct termios Opt;
	memset(&Opt ,0 ,sizeof(Opt));
	tcgetattr(fd, &Opt);
	cfsetispeed(&Opt,B115200);     /*设置为115200Bps*/
	cfsetospeed(&Opt,B115200);
	Opt.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); //特殊字符不做特殊处理
	Opt.c_oflag &= ~OPOST;
	Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	Opt.c_cflag |= CLOCAL | CREAD;//保证程序不会成为端的占有者|使端口能读取输入的数据
	Opt.c_cflag &= ~CSIZE;
	Opt.c_cflag |= CS8;		//8数据位
	Opt.c_cflag &= ~PARENB; //无校验
	Opt.c_cflag &= ~CSTOPB; //一停止位
	Opt.c_cc[VTIME] = 10;/* 非规范模式读取时的超时时间*/
	Opt.c_cc[VMIN]  = 128; /* 非规范模式读取时的最小字符数*/
	tcflush(fd ,TCIFLUSH);/* tcflush清空终端未完成的输入/输出请求及数据*/
	tcsetattr(fd,TCSANOW,&Opt);
}

/*Modbus初始化函数
 *@method Modbus_Init
 *@param{devAddr}设备地址
 *@return {void}
*/
int Modbus_Init(char *devAddr)
{
	fd = open(devAddr,O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd<0) return fd;
	Usart_Init();
	return 0;
}

/********************************************************
	【函数名称】 CRC16
	【函数功能】 生成CRC16-MODBUS校验码
	【参数】 
		buf 需要校验的数据
		size 数据长度
	【返回值】 16位校验值
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
unsigned short CRC16(unsigned char *buf,int size)
{
	unsigned short tmp = 0xffff;//CRC初始寄存
    unsigned short ret1 = 0;
	for(int n = 0; n < size; n++){/*此处的6 -- 要校验的位数为6个*/
        tmp = buf[n] ^ tmp;
        for(int i = 0;i < 8;i++){  /*此处的8 -- 指每一个char类型又8bit，每bit都要处理*/
            if(tmp & 0x01){
                tmp = tmp >> 1;
                tmp = tmp ^ 0xa001; //CRC16/MODBUS多项式：0xA001
            }   
            else{
                tmp = tmp >> 1;
            }   
        }   
    } 
	ret1 = tmp>>8;
	ret1 |= (tmp<<8);
	return ret1;
}

void HexViewer(uint8_t *buf, int rc){
	int i = 0;
	while (i<rc){
		printf("%02X ",buf[i++]);
	}
	printf("\n");
}

 /********************************************************
	【函数名称】 Modbus_SendCMD
	【函数功能】 发送Modbus命令
	【参数】 
		ID  	: 从机ID，范围0~255
		func  	: 功能码
		PDU		: PDU数据部分
		size    ：PDU数据长度
	【返回值】 发送数据实际长度
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
int Modbus_SendCMD(unsigned char ID,unsigned char func,unsigned char *PDU,int size)
{
	unsigned char buf[30] = {0};
	unsigned short crc = 0;
	buf[0] = 2 + size;
	buf[1] = ID;
	buf[2] = func;
	for (size-=1;size>=0;size--)
	{
		buf[3+size] = PDU[size];
	}
	crc = CRC16(buf+1,*buf);
	buf[(*buf)+1] = crc>>8;
	buf[(*buf)+2] = crc&0x00ff;
	buf[0] += 2;
	// printf("CMD Frame:");
	// HexViewer(buf,buf[0]);
	return write(fd,buf+1,*buf);
}

/********************************************************
	【函数名称】 Modbus_ReadReg
	【函数功能】 读取ModBus总线上的响应数据
	【参数】 无
	【返回值】 大于0表示读取到buffer的数据长度，等于0表示读取超时，-1表示读取错误
	【描述】 读取等待10毫秒，超时返回0
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
		2020-09-10 将持续等待改为10ms等待。
********************************************************/
int Modbus_ReadReg()
{
	fd_set mb_ReadFlag;//串口有可读数据时的标志bitmap
	struct timeval overTime = {0, 10};
	int rt,rdcount;
	FD_ZERO(&mb_ReadFlag);//清空标志位
	FD_SET(fd,&mb_ReadFlag);
	rt = select(fd+1,&mb_ReadFlag,NULL,NULL,&overTime);
	if (rt < 0) return rt;//错误
	else if (rt == 0) return 0;//超时
	else if (FD_ISSET(fd,&mb_ReadFlag)){
		rdcount = read(fd,buffer+pos,20);
		return rdcount;
	}
	return -1;
}

/********************************************************
	【函数名称】 Modbus_Shutdown
	【函数功能】 关闭串口，终止modbus
	【参数】 无
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
********************************************************/
void Modbus_Shutdown()
{
	close(fd);
}


/*struct ModbusInfoStruct
{
	uint8_t slaveAddress;						//从机地址：0广播，1~47为单独的地址
	uint8_t functionCode;						//功能码
	uint16_t registerAddress;					//寄存器地址
	uint8_t baseDataBitSize;					//数据位长度，8,16,32
	double mappingCoefficient;					//映射比例
	uint8_t dataType;							//要转换的数据类型
};*/

/********************************************************
	【函数名称】 Modbus_PollFunc
	【函数功能】 执行ModBus的寄存器读取功能,发送读取命令后等待响应，如果超时或数据不完整则重试2次
	【参数】 
		protocolInfo为ModBus配置信息结构体指针
		data为返回的数据指针
	【返回值】 无
	【注意】 函数重复调用需要保证两次调用之间5ms的延时
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-04-10
	【更改记录】
		2020-09-10 增加超时重传机制，增加帧检错机制
********************************************************/
void Modbus_PollFunc(void *protocolInfo, void *data)
{
	uint8_t ID = ((struct ModbusInfoStruct *)protocolInfo)->slaveAddress;
	uint16_t RegAddr = ((struct ModbusInfoStruct *)protocolInfo)->registerAddress;
	uint8_t buf[4] = {(RegAddr>>8)&0x00ff,RegAddr&0x00ff,0x00,0x01};
	int tempData;
	pos = 0;
	Modbus_SendCMD(ID,0x03,buf,4);			//发送查询命令
	usleep(50000);
	int e = Modbus_ReadReg();				//接收数据
	uint8_t reCheck = 2;					//超时重接收次数
	if (e<0) {								//读错误
		printf("\033[45;33m Read Error :%d \033[0m\n",e);
		return;
	}
	while (e<7 && reCheck) {					//重接收
		e += Modbus_ReadReg();
		reCheck--;
	}
	// printf("Echo Frame:");
	// HexViewer(buffer,e);
	if (e<7&&reCheck==0) {					//超时错误
		printf("\033[45;33m TimeOut Error :%d \033[0m\n",e);
		return;
	}
	//printf("Size:%d     CheckTime:%d\t",e,reCheck);

	uint16_t CRC = CRC16(buffer,5);
	uint16_t CRC_Buffer = (int16_t)(buffer[5]<<8|0x00ff&buffer[6])&0xffff;
	//printf("CRC:%#04X-->%#04X   " ,CRC ,CRC_Buffer);
	if (CRC!=CRC_Buffer) {					//CRC校验
		printf("\033[45;33m CRC Error! \033[0m\n");
		return ;
	}
	tempData = (int16_t)(buffer[3]<<8|0x00ff&buffer[4]);
	//int tempData = 5;
	*(double*)data = ((struct ModbusInfoStruct *)protocolInfo)->mappingCoefficient*tempData;
	//*(double*)data = (double)tempData;
}


 /********************************************************
	【函数名称】 MechanicalArm_SendCMD
	【函数功能】 发送机械臂控制命令
	【参数】 
		buf  	: 机械臂控制命令字符串
	【返回值】 无
	【开发者及日期】 苏鑫（suxin1412@qq.com） 2020-10-15
********************************************************/
void MechanicalArm_SendCMD(char *buf)
{
	write(fd ,buf ,12);
}



