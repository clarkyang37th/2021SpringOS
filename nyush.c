//my2306 lab2
//reference: https://github.com/jmreyes/simple-c-shell
//           https://blog.csdn.net/str999_cn/article/details/78699724
//           https://man7.org/linux/man-pages/man7/signal.7.html

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <libgen.h> //for basename
#include <dirent.h>

#define LIMIT 256      // // token 最多数量
#define MAXLINE 1000   // max number of characters from user input
#define out_redirect 0 /* 输出重定向 */
#define in_redirect 1
//#define testpipe        2
//work to be done: link < and pipes, signals
// Shell pid, pgid, terminal modes

static char *currentDirectory;

pid_t pid;
typedef struct job
{
    char jobname[MAXLINE];
    pid_t pid;
    struct job *next; //指向直接后继元素的指针
} job;
job *head = NULL; //声明一个空的头指针

int cd_command(char *argv[]);
//int do_pipe (char ** argv1, char ** argv2);
void init();
void sighandler();
int foreground(char *argv[]);
void shellPrompt();

char line[MAXLINE]; //check the input cat abc.c
char origin[MAXLINE];
// char temp[100][MAXLINE];
// int numOfCmd = 0;
// int pidTable[100];

//int cd_command(char *argv[]);
/* 查找可执行程序 */
/** 头文件有必要吗？**/
//头文件是指定编译时包括的文件（如include）和定义一些代码或函数。
// 如只有一个源码文件，可没有头文件.h，定义直接写在源码文件的头部。
// 如有多个源码文件（包括.rc)，可将每个文件的重复的头部定义部分写成一个.h.
// 就是少写些代码，编译效果是一样的。

//file descriptors 0, 1 and 2 (aka STDIN_FILENO, STDOUT_FILENO and STDERR_FILENO)
// 文件描述符：Standard input	STDIN_FILENO	stdin
//          Standard output	STDOUT_FILENO	stdout
//          Standard error	STDERR_FILENO	stderr
void init()
{
    currentDirectory = (char *)calloc(1024, sizeof(char));
}
void sighandler()
{
    printf("\n");
    return;
}
int foreground(char *argv[])
{
    if (argv[1] == NULL || argv[2] != NULL)
    {
        write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
        return -1;
    }
    int number = atoi(argv[1]); //array to int
    int o = 0;                  //链表里总共有多少个
    job *pfgn, *pfgnf;
    pfgn = head;
    while (pfgn != NULL)
    {
        o++;
        pfgn = pfgn->next;
    }
    if (number > o || number <= 0)
    {
        write(2, "Error: invalid job\n", strlen("Error: invalid job\n"));
        return -1;
    }
    pfgn = head;
    pfgnf = head;
    for (int p = 1; p < number; p++)
    {
        pfgnf = pfgn;
        pfgn = pfgn->next;
    }
    kill(pfgn->pid, SIGCONT);
    //printf("%d\n", pfgn->pid);
    int a; //用来get waitpid 状态信息
    if (waitpid(pfgn->pid, &a, WUNTRACED) == -1)
    {
        return -1;
    }
    // if (WIFSTOPPED(a))//两种表达方式。
    // {
    //     ;
    // }
    // else
    // {
    //     if (number == 1)
    //     {
    //         head = head->next;
    //     }
    //     else
    //     {
    //         pfgnf->next = pfgn->next;
    //     }
    //     free(pfgn);
    // }
    // printf("a = %d\n", a);
    // printf("WEXITSTATUS(a) = %d\n", WEXITSTATUS(a));
    // printf("WTERMSIG(a) = %d\n", WTERMSIG(a));
    // printf("WSTOPSIG(a) = %d\n", WSTOPSIG(a));
    // printf("WIFEXITED(a) = %d\n", WIFEXITED(a));
    // printf("WIFSIGNALED(a) = %d\n", WIFSIGNALED(a));
    // printf("WIFSTOPP(a) = %d\n", WIFSTOPPED(a));
    if (!WIFSTOPPED(a))
    {
        if (number == 1)
        {
            head = head->next;
        }
        else
        {
            pfgnf->next = pfgn->next;
        }
        free(pfgn);
    }
    return 0;
}
// typedef struct job
// {
// int number;//存储整形元素
// struct Link *next;//指向直接后继元素的指针
// }job;
//void ppt_test()
//{
//    printf("...Ouch!\n");
//}

void shellPrompt()
{
    char *q;
    char buf[MAXLINE];

    getcwd(buf, MAXLINE);
    q = basename(buf);
    printf("[nyush %s]$ ", q);
}

int cd_command(char *argv[])
{
    if (argv[1] == NULL || argv[2] != NULL) //可能会出现 越界的问题? strtok_r? 这段没问题。
    {
        write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
        return -1;
    }
    else
    {
        if (chdir(argv[1]) == -1)
        {
            write(2, "Error: invalid directory\n", strlen("Error: invalid directory\n"));
            return -1;
        }
    }
    return 0;
}

int do_command(int argc, char *argv[], int isFirst)
{
    int fd;
    if (strcmp(argv[0], "exit") == 0)
    {
        int o = 0;
        job *pfgn;
        pfgn = head;
        while (pfgn != NULL)
        {
            o++;
            pfgn = pfgn->next;
        }
        if (o != 0)
        {
            write(2, "Error: there are suspended jobs\n", strlen("Error: there are suspended jobs\n"));
            return -1;
        }
        else
        {
            exit(0);
        }
    }

    //cd
    else if (strcmp(argv[0], "jobs") == 0)
    {
        if (argc != 1)
        {
            write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
            return -1;
        }
        if (head == NULL)
        {
            return 0;
        }
        else
        {
            job *pjobs;
            pjobs = head;
            int n = 1;
            while (pjobs != NULL)
            {
                printf("[%d] %s", n, pjobs->jobname);
                n++;
                pjobs = pjobs->next;
            }
        }
    }
    else if (strcmp(argv[0], "cd") == 0)
    {
        cd_command(argv);
    }

    //
    //lab2
    // else if (strcmp(argv[0],"lab2") == 0){
    //     printf("Jiayou!\n");
    //     return 1;
    // }
    else if (strcmp(argv[0], "fg") == 0)
    { //fg command, currently only spell check
        foreground(argv);
    }
    //    chmod 600 file – owner can read and write
    //    chmod 700 file – owner can read, write and execute
    //    chmod 666 file – all can read and write
    //    chmod 777 file – all can read, write and execute
    else
    {
        int pipeIndex =0; //指的是管道所在的位置，并非管道个数
        int pipeFlag = 0;
        int offset;
        char filenamein[255];
        char filenameout[255];
        // char filenameappend[255];
        int rein = 0;
        int reout = 0;
        int reappend = 0;
        int checki = 0;
        int checko = 0;
        int checka = 0;
        for (int i = 0; i < argc; i++)
        {
            if (strcmp(argv[i], "|") == 0)
            {
                pipeIndex = i;
                pipeFlag = 1;
            }
            if (strcmp(argv[i], "<<") == 0)
            {
                write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                return (-1);
            }
        }
        //打一个关于>的补丁
        for(int q = 0; q < pipeIndex; q++)
        {
            if (strcmp(argv[q], ">") == 0)
            {
                write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                return (-1);//第一次调用的时候就报错返回了。
            }
        }
        offset = pipeFlag == 1 ? pipeIndex + 1 : 0;
        for (int p = offset; p < argc; p++)
        {
            if (strcmp(argv[p], "<") == 0)
            {
                if (checki)
                //cat < input.txt | cat < input2.txt | cat < output.txt
                //shell:is first == 1, 找到最右管道节，再从|符号开始处理子命令cat < output.txt
                //如果有check in < output.txt：符号删掉文件备份,然后进行fork argc argv -> shell waitpid
                //子进程1 递归的调用do command 来执行argc argv is first == 0// cat < output.txt
                {

                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    if (isFirst == 1)
                    {
                        return (-1);
                    }
                    exit(-1);
                }
                checki++;
            }
            if (strcmp(argv[p], ">") == 0)
            {
                if (isFirst == 0)
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    exit(-1);
                }
                if (checko)
                {

                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    if (isFirst == 1)
                    {
                        return (-1);
                    }
                    exit(-1);
                }
                checko++;
            }
            if (strcmp(argv[p], ">>") == 0)
            {
                if (isFirst == 0)
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    exit(-1);
                }
                if (checka)
                {

                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    if (isFirst == 1)
                    {
                        return (-1);
                    }
                    exit(-1);
                }
                checka++;
            }
        }
        if (pipeFlag == 1) //有管道,用来检查第二第三管道节是否有《 shell进程运行时不影响。
        {
            if (checki)
            {
                write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                if (isFirst == 1)
                {
                    return(-1);
                }
                exit(-1);
            }
            if (pipeIndex == 0 || pipeIndex == argc - 1)
            {
                write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                return (-1);
            }
        }
        //cat > a.txt < b.txt == cat < b.txt > a.txt
        //cat input.txt | < test.txt
        for (int l = offset; l < argc; l++)
        {
            if (strcmp(argv[l], "<") == 0)
            {
                rein = 1;
                if (l == argc - 1 || l == 0 || (pipeFlag == 1 && l == pipeIndex + 1))
                //previous :if (l == argc - 1 || l == 1)
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    return (-1);
                }
                strcpy(filenamein, argv[l + 1]);
                for (int m = l + 2; m < argc; m++)
                {
                    argv[m - 2] = argv[m];
                }
                argv[argc - 2] = NULL;
                argc -= 2;
                if (argc == 0) //处理过后没有东西
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    return (-1);
                }
                if (argv[l] != NULL)
                {
                    if (strcmp(argv[l], "|") != 0 && strcmp(argv[l], ">>") != 0 && strcmp(argv[l], ">") != 0)
                    {
                        write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                        return (-1);
                    }
                }
            }
        }

        for (int j = offset; j < argc; j++)
        {
            if (strcmp(argv[j], ">>") == 0)
            {
                reappend = 1;
                if (j == argc - 1 || j == 0 || (pipeFlag == 1 && j == pipeIndex + 1)) //防止在最后，最前和管道后的第一个
                //previous :if (l == argc - 1 || l == 1)
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    return (-1);
                }
                strcpy(filenameout, argv[j + 1]);
                for (int k = j + 2; k < argc; k++)
                {
                    argv[k - 2] = argv[k];
                }
                argv[argc - 2] = NULL;
                argc -= 2;
                if (argc == 0)
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    return (-1);
                }
                if (argv[j] != NULL)
                {
                    if (strcmp(argv[j], "|") != 0 && strcmp(argv[j], "<") != 0 && strcmp(argv[j], ">") != 0)
                    {
                        write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                        return (-1);
                    }
                }
                break;
            }

            if (strcmp(argv[j], ">") == 0)
            {
                reout = 1;
                if (j == argc - 1 || j == 0 || (pipeFlag == 1 && j == pipeIndex + 1))
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    return (-1);
                }
                strcpy(filenameout, argv[j + 1]);
                for (int k = j + 2; k < argc; k++)
                {
                    argv[k - 2] = argv[k];
                }
                argv[argc - 2] = NULL;
                argc -= 2;
                if (argc == 0)
                {
                    write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                    return (-1);
                }
                if (argv[j] != NULL)
                {
                    if (strcmp(argv[j], "|") != 0 && strcmp(argv[j], ">>") != 0 && strcmp(argv[j], "<") != 0)
                    {
                        write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                        return (-1);
                    }
                }
                break;
            }
        }
        if (isFirst == 0 && pipeFlag == 0)
        {
            if (rein == 1)
            {
                fd = open(filenamein, O_RDONLY);
                if (fd < 0)
                {
                    write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                    exit(-1);
                }
                dup2(fd, 0);
            }
            if (reout == 1)
            {
                fd = open(filenameout, O_RDWR | O_CREAT | O_TRUNC, 0644);
                if (fd < 0)
                {
                    write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                    exit(-1);
                }
                dup2(fd, 1);
            }
            if (reappend == 1)
            {
                fd = open(filenameout, O_RDWR | O_CREAT | O_APPEND, 0644);
                if (fd < 0)
                {
                    write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                    exit(-1);
                }
                dup2(fd, 1);
            }
            if (execvp(argv[0], argv) == -1)
            {
                write(2, "Error: invalid command\n", strlen("Error: invalid command\n"));
                exit(-1);
            } //执行第一个管道节的内容。
        }
        int pipefd[2];
        pipe(pipefd);
        if ((pid = fork()) < 0)
        {
            printf("fork error\n");
            return -1;
        }
        if (pid == 0)
        {
            if (isFirst == 1)
            {
                if (pipeFlag == 0)
                {
                    if (rein == 1)
                    {
                        fd = open(filenamein, O_RDONLY);
                        if (fd < 0)
                        {
                            write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                            exit(-1);
                        }
                        dup2(fd, 0);
                    }
                    if (reout == 1)
                    {
                        fd = open(filenameout, O_RDWR | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0)
                        {
                            write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                            exit(-1);
                        }
                        dup2(fd, 1);
                    }
                    if (reappend == 1)
                    {
                        fd = open(filenameout, O_RDWR | O_CREAT | O_APPEND, 0644);
                        if (fd < 0)
                        {
                            write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                            exit(-1);
                        }
                        dup2(fd, 1);
                    }
                    if (execvp(argv[0], argv) == -1)
                    {
                        write(2, "Error: invalid program\n", strlen("Error: invalid program\n"));
                        exit(-1);
                    }
                }
                else
                {
                    if (rein == 1)
                    {
                        fd = open(filenamein, O_RDONLY);
                        if (fd < 0)
                        {
                            write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                            return (-1);
                        }
                        dup2(fd, 0);
                    }
                    if (reout == 1)
                    {
                        fd = open(filenameout, O_RDWR | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0)
                        {
                            write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                            return (-1);
                        }
                        dup2(fd, 1);
                    }
                    if (reappend == 1)
                    {
                        fd = open(filenameout, O_RDWR | O_CREAT | O_APPEND, 0644);
                        if (fd < 0)
                        {
                            write(2, "Error: invalid file\n", strlen("Error: invalid file\n"));
                            return (-1);
                        }
                        dup2(fd, 1);
                    }
                    do_command(argc, argv, 0); // 删除了最后的重定向文件两个参数。
                }
            }
            else
            {
                //pipe fd里面0是读，1是写
                dup2(pipefd[1], 1);
                close(pipefd[0]);
                close(pipefd[1]);
                argv[pipeIndex] = NULL;
                do_command(pipeIndex, argv, 0); // 然后把pipeindex的位置置为NULL,表示只有这么多命令参数。只删除最后一个管道节。
            }
        }
        else
        {
            if (isFirst == 1)
            {
                int a;
                if (waitpid(pid, &a, WUNTRACED) == -1)
                {
                    return -1;
                }
                else
                {
                    if (WIFSTOPPED(a))
                    {
                        job *p = (job *)malloc(sizeof(job));
                        job *pb;
                        strcpy(p->jobname, origin);
                        p->pid = pid;
                        p->next = NULL;
                        if (head == NULL)
                        {
                            head = p;
                        }
                        else
                        {
                            pb = head;
                            while (pb->next != NULL) //循环查找要删除的节点
                            {
                                pb = pb->next;
                            }
                            pb->next = p;
                        }
                    }
                    return 0;
                }
            }
            else
            {
                dup2(pipefd[0], 0);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execvp(argv[pipeIndex + 1], argv + pipeIndex + 1) == -1)
                {
                    write(2, "Error: invalid program\n", strlen("Error: invalid program\n"));
                    exit(-1);
                }
                //&a[n] = a + n
                //a[n] = *(a+n)
            }
        }
    }

    return 0;
}

int main()
{

    char *tokens[LIMIT]; //check all the tokens
    int numTokens;

    pid = -10; // set an unavailable pid

    init();
    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGTSTP, sighandler);

    // The main loop
    while (1)
    {

        shellPrompt();
        fflush(stdout);

        // empty line buffer
        memset(line, '\0', MAXLINE);

        char *ret = fgets(line, MAXLINE, stdin);
        if (ret == NULL) //fgetc:from file get char fgets: get string//&& fgetc(stdin) == EOF 如果不输入char，会卡住,等待输入
        {
            printf("\n"); //起到了一个刷新缓存的作用
            break;        //打破循环用的;
        }
        strcpy(origin, line);
        //如果成功，则返回 line[0]地址，其实就是line,
        // 如果没有输入，则loopagain）
        //line:asdf b c df
        //cat abc.c
        //strtok 之后直接是cat
        if ((tokens[0] = strtok(line, " \n\t")) == NULL) //调用strtok抠出来，赋值给tokens0（括号先执行），然后判断括号里面是否为true（tokens 0 == NULL）
        {
            continue; //continue;
        }             //strtok读取非0串 cat lab2.c
                      //line: b c d tokens[0] = asdf  cat lab2.c//抠掉所有空格，留下有用的字符/t 是tab
                      //strtok第二个参数；把什么也当作空格，如果放入b，则b也会被处理掉；

        numTokens = 1;
        while ((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL)
            numTokens++;
        do_command(numTokens, tokens, 1);
    }
    exit(0);
}

//错误命令：lab2.c >> lab3.c？
//fd 相关
//signal handling
//printf("\n");
//shell fork sigstop 子进程hang，还是会继续执行wait
//jobs suspend cont suspend
//strtok 相关内容
//forground 和 jobs 会用到 origin
//c 语言的链表实现

//声明节点结构
// typedef struct Link{
// int elem;//存储整形元素
// struct Link *next;//指向直接后继元素的指针
// }link;
// //创建链表的函数
// link * initLink(){
// link * p=(link*)malloc(sizeof(link));//创建一个头结点
// link * temp=p;//声明一个指针指向头结点，用于遍历链表
// //生成链表
// for (int i=1; i<5; i++) {
// //创建节点并初始化
// link *a=(link*)malloc(sizeof(link));
// a->elem=i;
// a->next=NULL;
// //建立新节点与直接前驱节点的逻辑关系
// temp->next=a;
// temp=temp->next;
// }
// return p;
// }
//fg jobs 的链表实现？

//ps aux |grep sleep
//kill -STOP 870
