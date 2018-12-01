#include "client_func.h"

int File2Blocks(char *username, char *filename)
{
	char dirname[64] = FILE_HOME;                   //用户对应的本地操作文件的目录
	strcat(dirname, username);                      //每个具体用户对应的操作文件的目录
	strcat(dirname, "/");
	strcat(dirname, filename);
	printf("%s\n", dirname);
	FILE *fsrc = fopen(dirname, "rb");    			// 源文件
	
	char divname[64] = FILE_HOME;
	strcat(divname,username);
	strcat(divname,"/");
	sprintf(divname, "%s%s%s", divname, basename(filename), ".txt");
	printf("%s\n", divname);
	FILE *div = fopen(divname, "w");                // 存入分割条目的信息 
	
	if(fsrc == NULL || div == NULL)
	{
		perror("打开错误");
		return -1;
	}
	//获取文件的长度
	fseek(fsrc, 0, SEEK_END);
	int fLen = ftell(fsrc);
	printf("文件长度：%d\n", fLen);
	//每一块的长度
	int blockLen = fLen / FILE_BLOCKS;
	printf("blockLen:%d\n", blockLen);

	FILE *ftmp;   //临时文件

	for(int i = 0; i < FILE_BLOCKS; i++)  // 按块分割
	{
		char tName[20];
		char tdir[60];
		sprintf(tdir, "/home/w/store/%s/", username);
		sprintf(tName, "%s_part_%d", filename, i+1);	//生成文件名 
		strcat(tdir, tName); 					        //产生临时目录
		printf("%s\n", tdir);

		ftmp = fopen(tdir, "wb"); 				//生成临时文件
		if(ftmp == NULL)
		{ 
			perror("产生文件出错：");
			break; 
		} 
		fputs(tdir, div); 						//写入文件名
		fputc('\n',div); 
		int offset = i*blockLen; 				//计算偏移量 
		fseek(fsrc, offset, SEEK_SET); 
		int count = 0; 							//统计写入ftmp的数量 
		if(i == FILE_BLOCKS - 1)
			blockLen = fLen - blockLen*(FILE_BLOCKS - 1); 	//最后一块的长度
		while(count < blockLen && !feof(fsrc))
		{
			fputc(fgetc(fsrc),ftmp);
			count++;
		}
		printf("count:%d\n", count);
		fclose(ftmp); 
	}
	fclose(fsrc);
	fclose(div);
	return 0;
}

int Blocks2File(char *username, char *filename)
{
	char dirname[64] = FILE_HOME;                   //用户对应的本地操作文件的目录
	strcat(dirname, username);                      //每个具体用户对应的操作文件的目录
	strcat(dirname, "/");
	strcat(dirname, filename);
	printf("%s\n", dirname);
	FILE *fdest = fopen(dirname, "wb");    			// 源文件
	
	char divname[64] = FILE_HOME;
	strcat(divname,username);
	strcat(divname,"/");
	sprintf(divname, "%s%s%s", divname, basename(filename), ".txt");
	printf("%s\n", divname);
	FILE *div = fopen(divname, "r");                // 存入分割条目的信息 
	
	if(fdest == NULL || div == NULL)
	{
		perror("打开错误");
		return -1;
	}

	char tempName[60];
	FILE *tempFile;

	while(fgets(tempName, 60, div))
	{
		tempName[strlen(tempName)-1] = '\0';
		tempFile = fopen(tempName, "rb");
		if(tempFile == NULL)
		{
			printf("打开文件%s失败,", tempName);
			perror("出错原因");
			return -1;
		}
		printf("正在合并%s到新文件\n", tempName);
		int ch = fgetc(tempFile);
		while(!feof(tempFile))
		{
			fputc(ch, fdest);
			ch = fgetc(tempFile);
		}
		fclose(tempFile);
	}
	fclose(fdest);
	fclose(div);
	return 0;
}