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

//CRC16/MODBUS 校验码生成
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
/*****************
 *  ID  	: 从机ID，范围0~255
 *  func  	: 功能码
 *  PDU		: PDU数据部分
 *  size    ：PDU数据长度
 *****************/
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
	return write(fd,buf+1,*buf);
}

/********
 *	
 *
 *
 */
int Modbus_ReadReg(uint8_t ID,uint16_t RegAddr,int16_t *rn)
{
	fd_set mb_ReadFlag;//串口有可读数据时的标志bitmap
	int rt,rdcount;
	uint8_t buf[20] = {(RegAddr>>8)&0x00ff,RegAddr&0x00ff,0x00,0x01};
	printf("Reg:%x %x\t",buf[0],buf[1]);
	FD_ZERO(&mb_ReadFlag);//清空标志位
	FD_SET(fd,&mb_ReadFlag);
	Modbus_SendCMD(ID,0x03,buf,4);
	rt = select(fd+1,&mb_ReadFlag,NULL,NULL,NULL);
	if (rt <= 0) return -1;
	else if (FD_ISSET(fd,&mb_ReadFlag)){
		rdcount = read(fd,buf,20);
		printf("Reg:%x %x\t",buf[3],buf[4]);
		if (rn!=NULL) *rn = rdcount;
		return (int16_t)(buf[3]<<8|0x00ff&buf[4]);
	}
	return -1;
}
 
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
void Modbus_PollFunc(void *protocolInfo, void *data)
{
	printf("p->registerAddress:%d\t",((struct ModbusInfoStruct *)protocolInfo)->registerAddress);
	int tempData = Modbus_ReadReg(((struct ModbusInfoStruct *)protocolInfo)->slaveAddress,((struct ModbusInfoStruct *)protocolInfo)->registerAddress,NULL);
	//int tempData = 5;
	*(double*)data = ((struct ModbusInfoStruct *)protocolInfo)->mappingCoefficient*tempData;
}






