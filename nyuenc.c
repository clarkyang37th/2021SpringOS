#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "threadpool.h" //引用自己的内容不能用尖括号，要用双引号。
#define MAX_RLEN 1000
#define QUEUE_SIZE 1024
typedef struct node
{
    char letter;
    int num;
    struct node *next; //指向直接后继元素的指针
} node;

node *encode(char *src, int len)
{
    int rLen;

    /* If all characters in the source string are different,
    then size of destination string would be twice of input string.
    For example if the src is "abcd", then dest would be "a1b1c1d1"
    For other inputs, size would be less than twice.  */
    node *head = NULL;
    node *k = NULL;
    int i = 0;

    /* traverse the input string one by one */
    for (i = 0; i < len; i++)
    {
        node *temp = (node *)malloc(sizeof(node));
        temp->letter = src[i];
        rLen = 1;
        while (i + 1 < len && src[i] == src[i + 1])
        {
            rLen++;
            i++;
        }
        temp->num = rLen;
        temp->next = NULL;
        if (head == NULL)
        {
            head = temp;
        }
        else
        {
            k = head;
            while (k->next != NULL) //循环查找要删除的节点
            {
                k = k->next;
            }
            k->next = temp;
        }
    }

    /*terminate the destination string */
    return head;
}

typedef struct threadtask
{
    char *src;
    int len;
    node **result;
} task;

void multiencode(void *p)
{
    int rLen;
    task *t = (task *)p;

    node *head = NULL;
    node *k = NULL;
    int i = 0;

    /* traverse the input string one by one */
    for (i = 0; i < t->len; i++)
    {
        node *temp = (node *)malloc(sizeof(node));
        temp->letter = t->src[i];
        rLen = 1;
        while (i + 1 < t->len && t->src[i] == t->src[i + 1])
        {
            rLen++;
            i++;
        }
        temp->num = rLen;
        temp->next = NULL;
        if (head == NULL)
        {
            head = temp;
        }
        else
        {
            k = head;
            while (k->next != NULL) //循环查找要删除的节点
            {
                k = k->next;
            }
            k->next = temp;
        }
    }
    *(t->result) = head; //降级
}

/*driver program to test above function */
int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("Argc1 error\n");
        return -1;
    }
    if (strcmp(argv[1], "-j") == 0)
    {
        if (argc == 2)
        {
            printf("Argc 2 error\n");
            return -1;
        }

        int fd;
        struct stat input;
        int numofthreads;
        node *head = NULL;
        node *k = NULL;
        node *iter = NULL;
        char *addr;
        node ***all = malloc((argc-3)*sizeof(node**));
        char **allm = malloc((argc - 3) * sizeof(char *));

        if (strcmp(argv[2], "0") == 0)
        {
            printf("Seriously? Without one thread?\n");
            return -1;
        }
        if ((numofthreads = atoi(argv[2])) == 0)
        {
            printf("Number threads error\n");
            return -1;
        }
        threadpool_t *pool = threadpool_create(numofthreads, QUEUE_SIZE);
        for (int i = 3; i < argc; i++)
        {
            fd = open(argv[i], O_RDONLY); //打开第i个文件         
            if (fstat(fd, &input) == -1)
            {
                perror("fstat");
                exit(EXIT_FAILURE);
            }
            if (input.st_size == 0)
            {
                continue;
            }
            
            //创建结果保存空间：
            // int size = input.st_size/4096;
            // if (input.st_size % 4096 != 0)
            // {
            //     size++;
            // }
            int size = (input.st_size % 4096 == 0) ? (input.st_size/4096) : (input.st_size/4096 + 1);
            //记录每一行开头的一个地址对于每一行来说分配正好大小的空间
            all[i-3] = malloc((size + 1) * sizeof(node*));//这是一个二级指针
            allm[i - 3] = malloc(input.st_size * sizeof(char));
            all[i-3][size] = NULL; 
            addr = mmap(NULL, input.st_size, PROT_READ, MAP_SHARED, fd, 0);
            strncpy(allm[i - 3], addr, input.st_size);

            for (int l = 0; l < size; l++)
            {
                task* temp = (task*)malloc(sizeof(task));
                temp->src = allm[i-3] + l * 4096;
                temp->result = &all[i-3][l];
                temp->len = (input.st_size % 4096 == 0) ? 4096 : ((l != size-1 ) ? 4096 : (input.st_size % 4096));
                // 没有检测add出错的情况，如果任务队列满的话，这里需要用while不断在出错的时候继续重试。
                while (threadpool_add(pool, multiencode, temp))
                ;
            }
            munmap(addr, input.st_size);
            close(fd);//为什么之前打开结果是正确的？
        }
        threadpool_destroy(pool);
        //m == 3, n == 1, 2 3;
        for(int m = 0; m < (argc-3); m++)
        {
            for(int n = 0; all[m][n] != NULL; n++)
            {
                if (head == NULL)
                {
                    head = all[m][n];
                    k = all[m][n];
                    while (k->next != NULL)//指向最后一个节点
                    {
                        k = k->next;
                    }
                }
                else
                {
                    if (k->letter == all[m][n]->letter)
                    {
                        k->num += all[m][n]->num;
                        k->next = all[m][n]->next;
                        // free(all[m][n]);
                    }
                    else
                    {
                        k->next = all[m][n];
                        //k = k->next;
                    }
                    while (k->next != NULL)
                    {
                        k = k->next;
                    }
                }
            }
        }
        for (int o = 0; o < argc - 3; o++)
        {
            free(all[o]);
        }
        free(all);
        iter = head;
        while (iter != NULL)
        {
            printf("%c%c", iter->letter, iter->num);
            iter = iter->next;
        }
        node* freeiter = head;
        while (freeiter != NULL)
        {
            iter = freeiter->next;
            free(freeiter);
            freeiter = iter;
        }
    }
    else
    {
        node *head = NULL;
        node *k = NULL;
        node *iter = NULL;
        struct stat input;
        char *addr;
        //int fstat(int fd, struct stat *buf);
        int fd; //fd:file descriptor
        for (int i = 1; i < argc; i++)
        {
            fd = open(argv[i], O_RDONLY); //打开最后一个文件，不适用于multi文件
            if (fstat(fd, &input) == -1)
            {
                perror("fstat");
                exit(EXIT_FAILURE);
            }
            addr = mmap(NULL, input.st_size, PROT_READ, MAP_SHARED, fd, 0);
            node *h = encode(addr, input.st_size); //eg: a15b15c150 c15 b20 a15
            if (head == NULL)
            {
                head = h;
                k = head;
                while (k->next != NULL)//指向最后一个节点
                {
                    k = k->next;
                }
                
            }
            else
            {
                if (k->letter == h->letter)
                {
                    k->num += h->num;
                    k->next = h->next;
                    free(h);
                }
                else
                {
                    k->next = h;
                }
                while (k->next != NULL)//指向最后一个节点
                {
                    k = k->next;
                }
            }
            munmap(addr, input.st_size);
            close(fd);
        }
        iter = head;
        while (iter != NULL)
        {
            printf("%c%c", iter->letter, iter->num);
            iter = iter->next;
        }
    }
    return 0;
}

//pthread 创建 mutex condition queue
//一维指针和数组的关系，指针的数组如何表现
//看代码没有锁的问题，helgrind用时很长。没定位到问题。