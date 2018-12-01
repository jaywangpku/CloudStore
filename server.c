#include "server_func.h"

int main(int argc,char *argv[])
{
    int ret_send;
    int ret_recv;

    int sfd;  
    int cfd;  

    sfd = tcp_init("0.0.0.0", PORT);    //监听所有ip
    if(-1 == sfd)
    {
        return 0;
    }
    //signalhandler();
    do  
    {
        pthread_t id2;
        tcp_info *info;
        cfd = tcp_accept(sfd);
        if(cfd < 0)
        {
            continue;
        }
        info = (tcp_info*)malloc(sizeof(tcp_info));
        info->sfd = sfd;
        info->cfd = cfd;
        ret_recv = pthread_create(&id2, NULL, pthread_recv, (void*)info);
        if(ret_recv != 0)
        {
            printf("create pthread error!\n");
        }
    }while(1);

    close(cfd);
    close(sfd);
    return 0;  
}