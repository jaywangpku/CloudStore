#ifndef SERVER_H
#define SERVER_H

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
#include <time.h>
#include <libgen.h>

#define MAX_CON 10
#define MAX_SIZE 1024

#define CMD_EXIT "exit"
#define CMD_DOWNLOAD_FILE "download"
#define CMD_UPLOAD_FILE "upload"

//#define FILE_HOME "/home/ubuntu/YunStore/"
#define FILE_HOME "/home/w/c/"

#define PORT 1234

static recv_message = 1;

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
    int sfd;
    int cfd;
    char filename[128];
}tcp_info;

int tcp_init(const char* ip, int port);
int tcp_accept(int sfd);
void *send_file(void *arg);
void *recv_file(void *arg);
void *pthread_recv(void *arg);
void signalhandler(void);
void printf_time(void);

#endif