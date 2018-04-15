#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#define STD_IN 0
#define STD_OUT 1
extern char **environ;
int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];//指针数组
	int pid=getpid();
	int pipe_fd[2];
	int n;
	int fin,fout;
	fin=dup(STD_IN);
	fout=dup(STD_OUT);//记录标准输入输出，便于恢复
    while (1) {
		int i,j;
		dup2(fin,STD_IN);
		dup2(fout,STD_OUT);
		cmd[0]='\0';
		for (i=0;i<128;i++) args[i]=NULL;//恢复原状
        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        /* 清理结尾的换行符 */       
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        /* 拆解命令行 */
        args[0] = cmd;
		if (args[0][0]=='\0') {
			args[0]=NULL;
			goto cmd;
		}
        for (i = 0; *args[i]; i++)
           	for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
               	if (*args[i+1] == ' ') {
                   	*args[i+1] = '\0';
					args[i+1]++;
                   	while(*args[i+1]==' ') args[i+1]++;//找到第一个不是空格的字符					
                   	break;
               	}
        args[i] = NULL;
		n=i;	
		/* 管道特性：
			根据管道符号|将指令分离，前面的指令通过fork出的子进程执行，执行完毕后马上返回；父进程继续分离指令。
			对于最后一个指令，由父进程执行，完毕后continue进入下一次循环，读取下一行指令。
		*/
		i=0;
		while (args[i] != NULL) {
			if (strcmp(args[i],"|")==0){
				pipe(pipe_fd);//管道创建
				pid=fork();//分叉
				if (pid==0){
					close(pipe_fd[0]);
		 			dup2(pipe_fd[1],STD_OUT);
					close(pipe_fd[1]);//重定向
					for (j=i;j<=n;j++) args[j]=NULL;//修改参数
					goto cmd;
				}
				else {				
					close(pipe_fd[1]);
					dup2(pipe_fd[0],STD_IN);
					close(pipe_fd[0]);//重定向
					wait(NULL);//等待子进程结束
					for(j=i+1;j<=n;j++)	args[j-i-1]=args[j];
					n=n-i-1;
					i=-1;//移动数组中的元素
				}
			}
			i++;
		} 	
        /* 没有输入命令 */
cmd:    if (!args[0]){
            if (pid==0) return 0;
			else continue;
		}
        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] && chdir(args[1])<0)
				puts("File not found");
			if (pid==0) return 0;
			else continue;
        }//cd命令
        if (strcmp(args[0], "pwd") == 0) {
            char wd[4096];
            puts(getcwd(wd, 4096));
            if (pid==0) return 0;
			else continue;
        }//pwd命令
        if (strcmp(args[0], "exit") == 0)
            return 0;
		//exit命令
		if (strcmp(args[0], "env") == 0) {
			char **e;
			e=environ;
			while (*e != NULL) {
				puts(*e);
				*e++;
			}
			if (pid==0) return 0;
			else continue;
		}//env命令
		if (strcmp(args[0], "export") ==0) {
			if (args[1]) {
				int i=0;
				int l=strlen(args[1]);
				while (args[1][i]!='=' && i<l) i++;
				if (i==l) {
					puts("Illegal expression.You should input : export NAME=VALUE");
					if (pid==0) return 0;
					else continue;
				}
				args[2] = args[1]+i+1;
				args[1][i] = '\0';//将表达式的两端分离为两个参数
				setenv(args[1],args[2],1);
			}
			if (pid==0) return 0;
			else continue;
		}//export命令
        /* 外部命令 */
        pid_t pid1 = fork();
		if (pid1 < 0){
			puts("Fork failed.");
			continue;
		}
        if (pid1 == 0) {
            /* 子进程 */
            execvp(args[0], args);
            /* execvp失败 */
			puts("Command not found.");
            return 255;
        }
        /* 父进程 */
        wait(NULL);
		if (pid==0) return 0;					
		else continue;		
    }
}
