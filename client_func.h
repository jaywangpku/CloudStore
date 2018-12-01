#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <libgen.h>
#include <time.h>

#define MAX_CON 10
#define MAX_SIZE 1024

#define FILE_BLOCKS 9
#define FILE_HOME "/home/w/store/"

#define CMD_EXIT "exit"
#define CMD_DOWNLOAD_FILE "download"
#define CMD_UPLOAD_FILE "upload"

#define SERVER_IP1 "192.168.124.128"
#define SERVER_IP2 "192.168.124.128"
#define SERVER_IP3 "192.168.124.128"  //"119.29.121.38"

#define PORT 1234

typedef enum tagCmdID
{
    CMD_INVALID = -1,
    CMD_FILE_EXIST,
    CMD_FILE_NOT_EXIST
}E_CMD_ID;

typedef struct tagClientCom
{
    E_CMD_ID cmd_id;
    long length;
}T_CLIENT_COM_HEADER;

typedef struct
{
    int cfd;
    char filename[128];
}file_information;

typedef struct
{
	int num;
	int sfd;
	char buf[64];
}Client_CMD;

int tcp_connect(const char* ip, int port);
void* recv_file(void *arg);
void* send_file(void *arg);
void* pthread_send(void *arg);
void printf_time(void);

//int File2Blocks(char *username, char *filename);
int Blocks2File(char *username, char *filename);

#endif