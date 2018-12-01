#include "client_func.h"

char IP[3][16] = { SERVER_IP1, SERVER_IP2, SERVER_IP3 };

int main(int argc,char *argv[])
{
	printf("请输入用户名:\t");
	char username[16];
	gets(username);       //获取当前操作用户的用户名

	while(1)
	{
		printf("请输入命令:\t");
		char buf[64];
		gets(buf);            //获取当前用户想要执行的命令

		char commond[16];
	    char filename[16];
	    char save_path[64];
	    int getfilenum = 0;

	    sscanf(buf, "%s %s %s %d", commond, filename, save_path, &getfilenum);

	    if(!strcmp(buf, CMD_EXIT))  //如果是退出命令
	    {
	        break;
	    }

	    //如果是上传文件操作
		if(!strcmp(commond, CMD_UPLOAD_FILE))
		{
			//对文件进行分片处理
		    if(File2Blocks(username,filename) == -1)
		    {
		    	printf("文件分片错误\n");
		    	return -1;
		    }

			int i = 0;
			for(i = 0; i< 9; i++)
			{
				int ret_send;
			    pthread_t id1;
			    int sfd = tcp_connect(IP[i/3], PORT);
			
			    Client_CMD *cmd;
			    cmd = (Client_CMD*)malloc(sizeof(Client_CMD));
			    cmd->sfd = sfd;
			    cmd->num = i+1;
			    
			    strcpy(cmd->buf, commond);
			    strcat(cmd->buf, " ");
			    strcat(cmd->buf, username);
			    strcat(cmd->buf, " ");
			    strcat(cmd->buf, filename);

			    //开启多线程进行发送
			    // ret_send = pthread_create(&id1, NULL, (void*)pthread_send, (void*)cmd);
			    // if(ret_send != 0)
			    // {
			    //     printf("create pthread error!\n");
			    // }
			    // else
			    // {
			    //     pthread_join(id1, NULL);
			    // }

			   	//单线程进行操作
			   	pthread_send((void*)cmd);
			    close(sfd);
			}
		}
		//如果是下载文件操作
		if(!strcmp(commond, CMD_DOWNLOAD_FILE))
		{
			int ret_send;
		    pthread_t id1;
		    int sfd = tcp_connect(IP[(getfilenum-1)/3], PORT);
		
		    Client_CMD *cmd;
		    cmd = (Client_CMD*)malloc(sizeof(Client_CMD));
		    cmd->sfd = sfd;
		    cmd->num = getfilenum;
		    
		    strcpy(cmd->buf, commond);
		    strcat(cmd->buf, " ");
		    strcat(cmd->buf, username);
		    strcat(cmd->buf, " ");
		    strcat(cmd->buf, filename);
		    strcat(cmd->buf, " ");
		    strcat(cmd->buf, save_path);

		   	//单线程进行操作
		   	pthread_send((void*)cmd);
		    close(sfd);
		}
	}
    return 0;  
}