#include "client_func.h"

int tcp_connect(const char* ip, int port)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd == -1)
    {
        printf("socket error: %s", strerror(errno));
        return -1;
    }
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(struct sockaddr));
    
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    
    if(connect(sfd, (struct sockaddr*) &serveraddr, sizeof(struct sockaddr)) == -1)
    {
        printf("connect error: %s", strerror(errno));
        close(sfd);
        return -1;
    }
    printf("连接服务器成功..\n");
    return sfd;
}

void *recv_file(void *arg)
{
    printf("开始下载文件..\n");
    printf_time();
    int needRecv = sizeof(T_CLIENT_COM_HEADER);
    int length = 0;
    long pos = 0;

    char *head = (char*)malloc(needRecv);
    char buffer[MAX_SIZE];

    file_information *file_info;
    T_CLIENT_COM_HEADER *myNode = (T_CLIENT_COM_HEADER*)malloc(sizeof(T_CLIENT_COM_HEADER));

    file_info = (file_information *)arg;
    char *filename = basename(file_info->filename);
    memset(&buffer, 0, MAX_SIZE);

    while(pos < needRecv)
    {
        length = recv(file_info->cfd, head+pos, needRecv, 0);
        if (length < 0)
        {
            printf("Server Recieve Data Failed!\n");
            break;
        }
        pos += length;
    }
    memcpy(myNode,head,needRecv);
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
        fclose(fp);
    }
    else if(myNode->cmd_id == CMD_FILE_NOT_EXIST)
    {
        printf("File %s is not exist!\n", file_info->filename);
    }
    free(head);
    free(myNode);
}

void *send_file(void *arg)
{
    printf("开始上传文件..\n");
    printf_time();
    int needSend = sizeof(T_CLIENT_COM_HEADER);
    char *head = (char*)malloc(needSend);
    char buffer[MAX_SIZE];
    T_CLIENT_COM_HEADER *com = (T_CLIENT_COM_HEADER*)malloc(sizeof(T_CLIENT_COM_HEADER));
    file_information *file_info;

    file_info = (file_information *)arg;
    com->length = needSend;
    memset(buffer, 0, sizeof(buffer));

    if(access(file_info->filename, F_OK) >= 0)
    {
        FILE *fp = fopen(file_info->filename, "r");
        fseek(fp, 0L, SEEK_END);
        long file_size = ftell(fp);

        rewind(fp);

        com->length += file_size;
        com->cmd_id = CMD_FILE_EXIST;

        memcpy(head, com, needSend);
        int sendlength = send(file_info->cfd, head, needSend, 0);
        printf("head->cmd_id = %d, head->length = %ld\n", com->cmd_id, com->length);
        do
        {
            int file_block_length = fread(buffer, sizeof(char), MAX_SIZE, fp);
            int len = send(file_info->cfd, buffer, file_block_length, 0);
            if(file_block_length <= 0)  
            {
                break;
            }
            bzero(buffer, sizeof(buffer));
        }while(1);

        fclose(fp);  
        printf("File:\t%s \nTransfer Finished!\n", file_info->filename);
        printf_time();
    }
    else
    {
        com->cmd_id = CMD_FILE_NOT_EXIST;
        memcpy(head, com, needSend);
        int sendlength = send(file_info->cfd, head, needSend, 0);
    }
    close(file_info->cfd);
}

void *pthread_send(void *arg)   //获取指令，执行相应的操作
{
    Client_CMD *temp;
    char buf[128];

    memset(buf, 0, sizeof(buf));
    temp = (Client_CMD *)arg;
    strcpy(buf, temp->buf);

    char commond[64];
    char username[64];
    char filename[64];
    char save_path[64];

    int arg_cont;
    arg_cont = sscanf(buf, "%s %s %s %s", commond, username, filename, save_path);

    printf("commond %s\n", commond);
    printf("username %s\n", username);
    printf("filename %s\n", filename);
    printf("save_path %s\n", save_path);

    printf("arg_cont %d\n", arg_cont);

    //向服务器发送的字符信息
    sprintf(buf, "%s %s %s%s%d", commond, username, filename, "_part_", temp->num);
    printf("buf: %s\n",buf);
    if(send(temp->sfd, buf, sizeof(buf), 0) == -1)    //向服务器发送指令
    {
        printf("send error: %s", strerror(errno));
        close(temp->sfd);
        return;
    }

    //判断是否要进行下载文件操作
    if(!strcmp(commond, CMD_DOWNLOAD_FILE))
    {
        if(arg_cont != 4)
        {
            printf("parameter error!\n");
            return;
        }
        file_information *file_info;
        pthread_t pid;
        int ret;

        file_info = (file_information *)malloc(sizeof(file_information));
        memset(file_info, 0, sizeof(file_information));

        strcpy(file_info->filename, save_path);
        file_info->cfd = temp->sfd;

        // ret = pthread_create(&pid, NULL, (void*)recv_file, (void*)file_info);
        // if(ret != 0)
        // {
        //     printf("create pthread error!\n");
        // }
        // 采用单线程发送
        recv_file((void*)file_info);
    }
    //判断是否要进行上传文件操作   一个线程一次只传送一个文件
    if(!strcmp(commond, CMD_UPLOAD_FILE))
    {
        if(arg_cont != 3)
        {
            printf("parameter error!\n");
            return;
        }
        file_information *file_info;
        pthread_t pid;
        int ret;

        file_info = (file_information *)malloc(sizeof(file_information));
        memset(file_info, 0, sizeof(file_information));

        //合成上传所需要的文件名
        sprintf(file_info->filename, "%s%s%s%s%s%d", FILE_HOME, username, "/", filename, "_part_", temp->num);
        file_info->cfd = temp->sfd;

        // 采用多线程发送
        // ret = pthread_create(&pid, NULL, (void*)send_file, (void*)file_info);
        // if(ret != 0)
        // {
        //     printf("create pthread error!\n");
        // }
        // 采用单线程发送
        send_file((void*)file_info);
    }
    //close(temp->sfd);
    //pthread_exit(NULL);    //单线程操作不可以退出
}

void printf_time(void)
{
    time_t t;
    time(&t);
    printf("此时的时间是: %s", ctime(&t));
}