#include "server_func.h"

int tcp_init(const char* ip, int port)  //用于初始化
{
    int on = 1;
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd == -1)
    {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }
    int ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); 
    if(ret < 0)
    {
        printf("setsockopt error: %s", strerror(errno));
    }
    struct sockaddr_in serveraddr;
    memset( &serveraddr, 0, sizeof(struct sockaddr)); 
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr)) == -1)
    {
        printf("Bind error: %s\n", strerror(errno));
        close(sfd);
        return -1;
    }
    printf("Bind OK\n");
    if(listen(sfd, MAX_CON) == -1)  //监听它 最大连接数为10
    {
        printf("Listen error: %s\n", strerror(errno));
        close(sfd);
        return -1;
    }
    printf("Listen OK\n");
    return sfd;
}

int tcp_accept(int sfd)
{
    struct sockaddr_in clientaddr;
    int addrlen = sizeof(struct sockaddr);
    int new_fd = accept(sfd, (struct sockaddr*)&clientaddr, &addrlen);

    memset(&clientaddr, 0, addrlen);

    if(new_fd == -1)
    {
        printf("accept error: %s\n", strerror(errno));
        sleep(1);
        return -1;
    }
    printf("Client%d(%s %d) success connect...\n", new_fd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    return new_fd;
}

void *send_file(void *arg)
{
    printf("开始下载文件..\n");
    printf_time();
    int needSend = sizeof(T_CLIENT_COM_HEADER);
    char *head = (char*)malloc(needSend);
    char buffer[MAX_SIZE];
    T_CLIENT_COM_HEADER *com = (T_CLIENT_COM_HEADER*)malloc(sizeof(T_CLIENT_COM_HEADER));
    tcp_info *client_info;

    client_info = (tcp_info *)arg;
    com->length = needSend;
    memset(buffer, 0, sizeof(buffer));

    if(access(client_info->filename, F_OK) >= 0)
    {
        FILE *fp = fopen(client_info->filename, "r");
        fseek(fp, 0L, SEEK_END);
        long file_size = ftell(fp);

        rewind(fp);

        com->length += file_size;
        com->cmd_id = CMD_FILE_EXIST;

        memcpy(head, com, needSend);

        int sendlength = send(client_info->cfd, head, needSend, 0);
        printf("head->cmd_id = %d, head->length = %ld\n", com->cmd_id, com->length);
        do
        {
            int file_block_length = fread(buffer, sizeof(char), MAX_SIZE, fp);
            int len = send(client_info->cfd, buffer, file_block_length, 0);
            if(file_block_length <= 0)  
            {
                break;
            }
            bzero(buffer, sizeof(buffer));
        }while(1);

        fclose(fp);  
        printf("File:\t%s \nTransfer Finished!\n", client_info->filename); 
        printf_time();
    }
    else
    {
        com->cmd_id = CMD_FILE_NOT_EXIST;
        memcpy(head, com, needSend);
        int sendlength = send(client_info->cfd, head, needSend, 0);
    }
}

void *recv_file(void *arg)
{
    printf("开始上传文件..\n");
    printf_time();
    int needRecv = sizeof(T_CLIENT_COM_HEADER);
    int length = 0;
    long pos = 0;

    char *head = (char*)malloc(needRecv);
    char buffer[MAX_SIZE];

    tcp_info *file_info;
    T_CLIENT_COM_HEADER *myNode = (T_CLIENT_COM_HEADER*)malloc(sizeof(T_CLIENT_COM_HEADER));

    file_info = (tcp_info *)arg;
    char *filename = basename(file_info->filename);
    memset(&buffer, 0, MAX_SIZE);
    
    printf("needRecv %d\n", needRecv);
    while(pos < needRecv)
    {
        length = recv(file_info->cfd, head+pos, needRecv, 0);
        printf("length %d\n", length);
        if (length < 0)
        {
            printf("Server Recieve Data Failed!\n");
            break;
        }
        pos += length;
    }
    memcpy(myNode,head,needRecv);
    printf("head->cmd_id = %d, head->length = %ld\n", myNode->cmd_id, myNode->length);
    if(myNode->cmd_id == CMD_FILE_EXIST)
    {
        printf("File %s is sending...\n", filename);
        filename = NULL;
        FILE *fp = fopen(file_info->filename, "w");
        length = 0;
        pos = 0;
        if(fp == NULL)
        {
            printf("File:\t%s Can Not Open To Write!\n", file_info->filename);
            return;
        }

        while(pos < myNode->length - needRecv)
        {
            int write_length;
            length = recv(file_info->cfd, buffer, MAX_SIZE, 0);
            if(length < 0)
            {
                printf("Recieve Data From Server Failed!\n");
                break;
            }

            pos += length;
            write_length = fwrite(buffer, sizeof(char), length, fp);

            if(write_length < length)
            {
                printf("File:\t%s Write Failed!\n", file_info->filename);
                break;
            }
            bzero(buffer, MAX_SIZE);
        }

        printf("Recieve File: %s\tFrom Server Finished!\n", file_info->filename);
        printf_time();
        recv_message = 1;   //使守护线程中的接收和接收文件信息的接收可以协调操作
        fclose(fp);
    }
    else if(myNode->cmd_id == CMD_FILE_NOT_EXIST)
    {
        printf("File %s is not exist!\n", file_info->filename);
    }
    free(head);
    free(myNode);
}

void *pthread_recv(void *arg)    //接收指令信息，判断需要进行的操作
{
    char buf[128] = {0};
    tcp_info *client_info;
    client_info = (tcp_info *)arg;
    memset(buf, 0, sizeof(buf));
    while(1)
    {
        char commond[64];
        char username[64];
        char filename[128];
        char save_path[128];
        int ret;
        if(recv_message == 1)   //使守护线程中的接收和接收文件信息的接收可以协调操作
        {
            ret = recv(client_info->cfd, buf, sizeof(buf), 0);    //获取指令信息buf
        }
        else
        {
            sleep(1);
            continue;
        }

        if(ret < 0)
        {
            printf("receive error: %s\n", strerror(errno));
            break;
        }
        else if(ret == 0)
        {
            printf("Client%d(Exception) exit!\n", client_info->cfd);
            break;
        }
        if(!strcmp(buf, CMD_EXIT))
        {
            printf("Client%d exit!\n", client_info->cfd);  
            break;  
        }
        printf("Client%d: %s\n", client_info->cfd, buf);
        //判断buf信息是否是下载文件的信息
        if((sscanf(buf, "%s %s %s", commond, username, filename) != 0) && (!strcmp(commond, CMD_DOWNLOAD_FILE)))
        {
            pthread_t pid;
            int ret;
            int length = 0;
            //制造服务器上的文件地址
            sprintf(client_info->filename, "%s%s%s%s", FILE_HOME, username, "/", filename);
            ret = pthread_create(&pid, NULL, send_file, (void*)client_info);
            continue; 
        }
        //判断buf信息是否是上传文件的信息
        if((sscanf(buf, "%s %s %s", commond, username, filename) != 0) && (!strcmp(commond, CMD_UPLOAD_FILE)))
        {
            pthread_t pid;
            int ret;
            int length = 0;
            //制造服务器上的存储路径
            sprintf(client_info->filename, "%s%s%s%s", FILE_HOME, username, "/", filename);
            printf("文件存储路径是:%s\n", client_info->filename);
            recv_message = 0;                       //使守护线程中的接收和接收文件信息的接收可以协调操作
            ret = pthread_create(&pid, NULL, recv_file, (void*)client_info);
            continue; 
        }
    }
    close(client_info->cfd);
    free(client_info);
    pthread_exit(NULL);
}

//阻塞信号
void signalhandler(void)
{
    sigset_t sigSet;
    sigemptyset(&sigSet);
    sigaddset(&sigSet,SIGINT);
    sigaddset(&sigSet,SIGQUIT);
    sigaddset(&sigSet,SIGPIPE);
    sigprocmask(SIG_BLOCK,&sigSet,NULL);   
}

void printf_time(void)
{
    time_t t;
    time(&t);
    printf("此时的时间是: %s", ctime(&t));
}