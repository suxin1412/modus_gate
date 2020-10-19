#This is OPCUA Gateway makefile
all:
	gcc -g -D_REENTRANT -I /usr/include/mariadb -o ./bin/gateway ./src/main.c ./src/OPCUAServer.c ./src/Polling.c ./src/modbus.c ./obj/cJSON.a ./obj/libopen62541.a -lpthread -lmysqlclient