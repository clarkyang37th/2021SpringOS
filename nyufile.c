#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <openssl/sha.h>
#define SHA_DIGEST_LENGTH 20
int a[20];//栈变量
int order;

//https://www.geeksforgeeks.org/write-a-c-program-to-print-all-permutations-of-a-given-string/
//https://www.jianshu.com/p/65adf0e476f7

#pragma pack(push,1)
typedef struct BootEntry {//这里是定义一个typedefine，不是定义一个变量 tef定义成一个名字
  unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
  unsigned char  BS_OEMName[8];     // OEM Name in ASCII

  unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096

  unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  
  unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area

  unsigned char  BPB_NumFATs;       // Number of FATs//
  
  unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
  unsigned char  BPB_Media;         // Media type
  unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
  unsigned short BPB_NumHeads;      // Number of heads in storage device
  unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0

  unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT

  unsigned short BPB_ExtFlags;      // A flag for FAT
  unsigned short BPB_FSVer;         // The major and minor version number
  unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
  unsigned char  BPB_Reserved[12];  // Reserved
  unsigned char  BS_DrvNum;         // BIOS INT13h drive number
  unsigned char  BS_Reserved1;      // Not used
  unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
  unsigned int   BS_VolID;          // Volume serial number
  unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
  unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;//是一个引用，所以肯定要加*
#pragma pack(pop)

#pragma pack(push,1)
typedef struct DirEntry {

  unsigned char  DIR_Name[11];      // File name

  unsigned char  DIR_Attr;          // File attributes
  unsigned char  DIR_NTRes;         // Reserved
  unsigned char  DIR_CrtTimeTenth;  // Created time (tenths of second)
  unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
  unsigned short DIR_CrtDate;       // Created day
  unsigned short DIR_LstAccDate;    // Accessed day

  unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address

  unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
  unsigned short DIR_WrtDate;       // Written day

  unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
  unsigned int   DIR_FileSize;      // File size in bytes. (0 for directories)

} DirEntry;
#pragma pack(pop)

//00000000 00000000 00000000 00000000

//00000000 00000000 00000000 00000000
int factor = 16*16*16*16;

int Commandi(char *arg)
{
    //首先open argv[1]
    char *addr;
    int fd;

    fd = open(arg, O_RDWR);//如果只读的话返回空指针
    if (fd < 0) {
        return -1;
    }

    struct stat buf;//sys/stat
    fstat(fd, &buf);//&取地址
    int filesize = buf.st_size;//取文件大小

    addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);//第二个参数是sizet length，这里取多少？bootsector大小吗？sector和cluster有什么区别？cluster 如果大于2 里面空间是否能用？
    
    struct BootEntry *BootEntry1;
    BootEntry1 = (struct BootEntry*)addr;
    printf("Number of FATs = %d\n", BootEntry1->BPB_NumFATs);//都应该是%d, 理解成一个8bit int, 不要的是字符，要的的数字
    printf("Number of bytes per sector = %d\n", BootEntry1->BPB_BytsPerSec);
    printf("Number of sectors per cluster = %d\n", BootEntry1->BPB_SecPerClus);
    printf("Number of reserved sectors = %d\n", BootEntry1->BPB_RsvdSecCnt);

    munmap(addr, filesize);
    close(fd);
    return 0;
}

void printL(DirEntry * de)
{
    if(de->DIR_Attr == 0x10)
    {
        int k = 0;
        for ( ; k < 8; k++)//这里没有初始化的语句，分号需要用来表示
        {
            if (de->DIR_Name[k] == 0x20)//假设第四位是20的话，那么执行break时k = 3.
            {
                break;
            }
        }
        for (int l = 0; l < k; l++)//for 第二句代表的是为真的时候才会去执行for
        {
            printf("%c", de->DIR_Name[l]);
        }
        printf("/ (size = %d, starting cluster = %d)\n", de->DIR_FileSize, de->DIR_FstClusHI*factor + de->DIR_FstClusLO);
    }
    else
    {
        int k = 0;
        for ( ; k < 8; k++)
        {
            if (de->DIR_Name[k] == 0x20)//假设第四位是20的话，那么执行break时k = 3.
            {
                break;
            }
        }
        for (int l = 0; l < k; l++)
        {
            printf("%c", de->DIR_Name[l]);
        }
        if (de->DIR_Name[8] != 0x20)
        {
            printf(".%c",de->DIR_Name[8]);
            //printf(".%c%c%c (size = %d, starting cluster = %d)\n",de->DIR_Name[8], de->DIR_Name[9], de->DIR_Name[10], de->DIR_FileSize, de->DIR_FstClusHI*factor + de->DIR_FstClusLO);
        }
        if (de->DIR_Name[9] != 0x20)
        {
            printf("%c",de->DIR_Name[9]);
        }
        if (de->DIR_Name[10] != 0x20)
        {
            printf("%c",de->DIR_Name[10]);
        }
        printf(" (size = %d, starting cluster = %d)\n", de->DIR_FileSize, de->DIR_FstClusHI*factor + de->DIR_FstClusLO);
    }

}

void Recover(int * addrfat, DirEntry * de,BootEntry* be,  char* recfilename)//单个文件的恢复操作
{
    de->DIR_Name[0] = recfilename[0];
    int numOfCluster = de->DIR_FileSize/(be->BPB_BytsPerSec*be->BPB_SecPerClus);
    int modCluster = de->DIR_FileSize%(be->BPB_BytsPerSec*be->BPB_SecPerClus);
    if (modCluster != 0)
    {
        numOfCluster++;
    }
    int firstcluster = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
    for (int q = firstcluster; q < firstcluster+ numOfCluster-1; q++)//三个cluster, 456, first 是4
    {
        addrfat[q] = q+1;
    }
    addrfat[firstcluster+ numOfCluster-1] = 0x0ffffff8;//*(A+1) = A[1]
}

void RecoverForce(int * addrfat, DirEntry * de,BootEntry* be,  char* recfilename)
{
    de->DIR_Name[0] = recfilename[0];
    int numOfCluster = de->DIR_FileSize/(be->BPB_BytsPerSec*be->BPB_SecPerClus);
    int modCluster = de->DIR_FileSize%(be->BPB_BytsPerSec*be->BPB_SecPerClus);
    if (modCluster != 0)
    {
        numOfCluster++;
    }
    for (int q = 0; q < order-1; q++)//假设379 order == 3
    {

        addrfat[a[q]] = a[q+1];
    }
    addrfat[a[order -1]] = 0x0ffffff8;//*(A+1) = A[1]
}


int Check(DirEntry * de,  char* recfilename)
{
    int k = 0;
    int m = 8;

    char tempfilename[11];
    char bufferRecvFileName[11];//驼峰命名；
    memset(tempfilename, 0x20, 11);
    memset(bufferRecvFileName, 0x20, 11);

    for (; k < 8; k++)//这里没有初始化的语句，分号需要用来表示
    {
        if (de->DIR_Name[k] == 0x20)//假设第四位是20的话，那么执行break时k = 3.
        {
            break;
        }
    }

    for (; m < 11; m++)
    {
        if (de->DIR_Name[m] == 0x20)
        {
            break;
        }
    }
    //char tempfilename[11];//暂时的文件名，用来与recfilename进行比， 这是一个char数组-9:没有点
    
    for (int l = 1; l < k; l++)//这里没有初始化的语句，分号需要用来表示
    {
        tempfilename[l-1] = de->DIR_Name[l];
    }
    
    if(m != 8)
    {
        tempfilename[k-1] = '.';
        for (int n = 8; n < m ; n++)    //ello.txt202020
        {
            tempfilename[k+n-8] = de->DIR_Name[n];
        }
    }
    //printf("tempfilename = %s\n", tempfilename);
    
    int recLength = strlen(recfilename);
    
    for (int p = 1; p < recLength; p++) //length = 3
    {
        bufferRecvFileName[p-1] = recfilename[p];
    }
    //printf("bufferRecvFileName = %s\n", bufferRecvFileName);

    //?数组也是一个指针，数组=第0个元素开头的地址.
    if (strncmp(tempfilename, bufferRecvFileName, 11) == 0)//找到了名称相似的文件
    {
        return 0;
    }
    else
    {
        return (-1);
    }
}

int CheckSha(int * addrfat, DirEntry * de, BootEntry* be,  char* recfilename, char* sha)
{
    int k = 0;
    int m = 8;
    

    char tempfilename[11];
    char bufferRecvFileName[11];//驼峰命名；
    char * fileContent;
    memset(tempfilename, 0x20, 11);
    memset(bufferRecvFileName, 0x20, 11);
    char *addrdata = ((char*)addrfat) + (be->BPB_NumFATs * be->BPB_FATSz32 * be->BPB_BytsPerSec);

    for (; k < 8; k++)//这里没有初始化的语句，分号需要用来表示
    {
        if (de->DIR_Name[k] == 0x20)//假设第四位是20的话，那么执行break时k = 3.
        {
            break;
        }
    }

    for (; m < 11; m++)
    {
        if (de->DIR_Name[m] == 0x20)
        {
            break;
        }
    }
    
    for (int l = 1; l < k; l++)//这里没有初始化的语句，分号需要用来表示
    {
        tempfilename[l-1] = de->DIR_Name[l];
    }
    
    if(m != 8)
    {
        tempfilename[k-1] = '.';
        for (int n = 8; n < m ; n++)    //ello.txt202020
        {
            tempfilename[k+n-8] = de->DIR_Name[n];
        }
    }
    //printf("tempfilename = %s\n", tempfilename);
    
    int recLength = strlen(recfilename);
    
    for (int p = 1; p < recLength; p++) //length = 3
    {
        bufferRecvFileName[p-1] = recfilename[p];
    }
    //printf("bufferRecvFileName = %s\n", bufferRecvFileName);

    //?数组也是一个指针，数组=第0个元素开头的地址.
    if (strncmp(tempfilename, bufferRecvFileName, 11) == 0)//找到了名称相似的文件
    {
        unsigned char obuf[SHA_DIGEST_LENGTH];
        fileContent = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(de->DIR_FstClusHI*factor + de->DIR_FstClusLO - 2);
        SHA1((unsigned char*)fileContent, de->DIR_FileSize, obuf);
        if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
        {
            return 0;//代表文件符合要求
        }
        else
        {
            return -1;
        }

    }
    else
    {
        return -1;
    }
}

// void swap(int *x, int *y)
// {
//     int temp;
//     temp = *x;
//     *x = *y;
//     *y = temp;
// }

// void permute(int *a, int l, int r)
// {
//     int i;
//     if (l == r)
//         printf("%s\n", a);
//     else
//     {
//         for (i = l; i <= r; i++)
//         {
//             swap((a+l), (a+i));
//             permute(a, l+1, r);
//             swap((a+l), (a+i)); //backtrack
//         }
//     }
// }
 
/* Driver program to test above functions */
// int main()
// {
//     char str[] = "ABC";
//     int n = strlen(str);
//     permute(str, 0, n-1);
//     return 0;
// }

int CheckShaForce(int * addrfat, DirEntry * de, BootEntry* be,  char* recfilename, char* sha)
{
    int k = 0;
    int m = 8;
    

    char tempfilename[11];
    char bufferRecvFileName[11];//驼峰命名；
    char * fileContent;
    char * tempCluster;
    memset(tempfilename, 0x20, 11);
    memset(bufferRecvFileName, 0x20, 11);
    char *addrdata = ((char*)addrfat) + (be->BPB_NumFATs * be->BPB_FATSz32 * be->BPB_BytsPerSec);
    unsigned char obuf[SHA_DIGEST_LENGTH];

    for (; k < 8; k++)//这里没有初始化的语句，分号需要用来表示
    {
        if (de->DIR_Name[k] == 0x20)//假设第四位是20的话，那么执行break时k = 3.
        {
            break;
        }
    }

    for (; m < 11; m++)
    {
        if (de->DIR_Name[m] == 0x20)
        {
            break;
        }
    }
    
    for (int l = 1; l < k; l++)//这里没有初始化的语句，分号需要用来表示
    {
        tempfilename[l-1] = de->DIR_Name[l];
    }
    
    if(m != 8)
    {
        tempfilename[k-1] = '.';
        for (int n = 8; n < m ; n++)    //ello.txt202020
        {
            tempfilename[k+n-8] = de->DIR_Name[n];
        }
    }
    //printf("tempfilename = %s\n", tempfilename);
    
    int recLength = strlen(recfilename);
    
    for (int p = 1; p < recLength; p++) //length = 3
    {
        bufferRecvFileName[p-1] = recfilename[p];
    }
    //printf("bufferRecvFileName = %s\n", bufferRecvFileName);

    //?数组也是一个指针，数组=第0个元素开头的地址.
    if (strncmp(tempfilename, bufferRecvFileName, 11) == 0)//找到了名称相似的文件
    {
        char guessContent[de->DIR_FileSize];//不能左边一个数组的声明，右边一个等号。

        fileContent = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(de->DIR_FstClusHI*factor + de->DIR_FstClusLO - 2);//第一个文件块

        strncpy(guessContent, fileContent, be->BPB_BytsPerSec * be->BPB_SecPerClus);
        int numOfCluster = de->DIR_FileSize/(be->BPB_BytsPerSec*be->BPB_SecPerClus);
        int modCluster = de->DIR_FileSize%(be->BPB_BytsPerSec*be->BPB_SecPerClus);
        if (modCluster != 0)
        {
            numOfCluster++;
        }
        for(int q = 2; q < 12; q++)
        {
            if (q == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
            {
                continue;
            }
            if (numOfCluster == 2)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-be->BPB_BytsPerSec * be->BPB_SecPerClus);
                //改的一直是后面388个字节，假设是900个字节的话
                SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                {
                    a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                    a[1] = q;
                    order = 2;
                    return 0;//代表文件符合要求
                }
            }
            else if (numOfCluster == 3)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第二个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-2*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                    if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                    {
                        a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                        a[1] = q;
                        a[2] = r;
                        order = 3;
                        return 0;//代表文件符合要求
                    }
                }
            }
            else if (numOfCluster == 4)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s < 12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-3*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                        if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                        {
                            a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                            a[1] = q;
                            a[2] = r;
                            a[3] = s;
                            order = 4;
                            return 0;//代表文件符合要求
                        }
                    }
                }
            }
            else if (numOfCluster == 5)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s < 12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        for (int t = 2; t < 12; t++)
                        {
                            if(t == q || t == r || t == s || t == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                            {
                                continue;
                            }
                            tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(t - 2);//假设第四个块的内容.
                            strncpy(guessContent + 4*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-4*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                            if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                            {
                                a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                                a[1] = q;
                                a[2] = r;
                                a[3] = s;
                                a[4] = t;
                                order = 5;
                                return 0;//代表文件符合要求
                            }
                        }
                    }
                }
            }
            else if (numOfCluster == 6)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s < 12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        for (int t = 2; t < 12; t++)
                        {
                            if(t == q || t == r || t == s || t == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                            {
                                continue;
                            }
                            tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(t - 2);//假设第四个块的内容.
                            strncpy(guessContent + 4*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            for (int u = 2; u < 12; u++)
                            {
                                if(u == q || u == r || u == s || u ==t || u == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                {
                                    continue;
                                }
                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(u - 2);//假设第四个块的内容.
                                strncpy(guessContent + 5*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-5*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                                if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                                {
                                    a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                                    a[1] = q;
                                    a[2] = r;
                                    a[3] = s;
                                    a[4] = t;
                                    a[5] = u;
                                    order = 6;
                                    return 0;//代表文件符合要求
                                }
                            }
                        }
                    }
                }
            }
            else if (numOfCluster == 7)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s <12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        for (int t = 2; t < 12; t++)
                        {
                            if(t == q || t == r || t == s || t == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                            {
                                continue;
                            }
                            tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(t - 2);//假设第四个块的内容.
                            strncpy(guessContent + 4*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            for (int u = 2; u < 12; u++)
                            {
                                if(u == q || u == r || u == s || u ==t || u == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                {
                                    continue;
                                }
                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(u - 2);//假设第四个块的内容.
                                strncpy(guessContent + 5*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                for (int v = 2; v < 12; v++)
                                {
                                    if(v == q || v == r || v == s || v == t || v == u || v == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                    {
                                        continue;
                                    }//改动内容：numOfCluster更改，上一篇的结尾strncpysize 新建for循环放进去， 添加一个continue条件，, tempcluster 内容更改，strncpy+1:size更改。
                                    tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(v - 2);//假设第四个块的内容.
                                    strncpy(guessContent + 6*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-6*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                    SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                                    if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                                    {
                                        a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                                        a[1] = q;
                                        a[2] = r;
                                        a[3] = s;
                                        a[4] = t;
                                        a[5] = u;
                                        a[6] = v;
                                        order = 7;
                                        return 0;//代表文件符合要求
                                    }
                                }
                            }
                        }   
                    }
                }
            }
            else if (numOfCluster == 8)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s < 12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        for (int t = 2; t < 12; t++)
                        {
                            if(t == q || t == r || t == s || t == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                            {
                                continue;
                            }
                            tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(t - 2);//假设第四个块的内容.
                            strncpy(guessContent + 4*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            for (int u = 2; u < 12; u++)
                            {
                                if(u == q || u == r || u == s || u ==t || u == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                {
                                    continue;
                                }
                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(u - 2);//假设第四个块的内容.
                                strncpy(guessContent + 5*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                for (int v = 2; v < 12; v++)
                                {
                                    if(v == q || v == r || v == s || v == t || v == u || v == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                    {
                                        continue;
                                    }
                                    //改动内容：numOfCluster更改，上一篇的结尾strncpysize 新建for循环放进去， 添加一个continue条件，, tempcluster 内容更改，strncpy+1:size更改。
                                    tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(v - 2);//假设第四个块的内容.
                                    strncpy(guessContent + 6*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster,be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                    for (int w = 2; w < 12; w++)
                                    {
                                        if(w == q || w == r || w == s || w == t || w == u || w == v || w == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                        {
                                            continue;
                                        }
                                        tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(w - 2);//假设第四个块的内容.
                                        strncpy(guessContent + 7*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-7*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                        SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                                        if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                                        {
                                            a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                                            a[1] = q;
                                            a[2] = r;
                                            a[3] = s;
                                            a[4] = t;
                                            a[5] = u;
                                            a[6] = v;
                                            a[7] = w;
                                            order = 8;
                                            return 0;//代表文件符合要求
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (numOfCluster == 9)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s < 12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        for (int t = 2; t < 12; t++)
                        {
                            if(t == q || t == r || t == s || t == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                            {
                                continue;
                            }
                            tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(t - 2);//假设第四个块的内容.
                            strncpy(guessContent + 4*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            for (int u = 2; u < 12; u++)
                            {
                                if(u == q || u == r || u == s || u ==t || u == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                {
                                    continue;
                                }
                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(u - 2);//假设第四个块的内容.
                                strncpy(guessContent + 5*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                for (int v = 2; v < 14; v++)
                                {
                                    if(v == q || v == r || v == s || v == t || v == u || v == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                    {
                                        continue;
                                    }
                                    //改动内容：numOfCluster更改，上一篇的结尾strncpysize改回512 新建for循环放进去， 添加一个continue条件，, tempcluster 内容更改，strncpy+1:size更改。
                                    tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(v - 2);//假设第四个块的内容.
                                    strncpy(guessContent + 6*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster,be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                    for (int w = 2; w < 12; w++)
                                    {
                                        if(w == q || w == r || w == s || w == t || w == u || w == v || w == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                        {
                                            continue;
                                        }
                                        tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(w - 2);//假设第四个块的内容.
                                        strncpy(guessContent + 7*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                        for(int x = 2; x < 12; x++)
                                        {
                                            if(x == q || x == r || x == s || x == t || x == u || x == v || x == w || x == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                            {
                                                continue;
                                            }
                                            tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(x - 2);//假设第四个块的内容.
                                            strncpy(guessContent + 8*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-8*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                            SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                                            if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                                            {
                                                a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                                                a[1] = q;
                                                a[2] = r;
                                                a[3] = s;
                                                a[4] = t;
                                                a[5] = u;
                                                a[6] = v;
                                                a[7] = w;
                                                a[8] = x;
                                                order = 9;
                                                return 0;//代表文件符合要求
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (numOfCluster == 10)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s < 12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        for (int t = 2; t < 12; t++)
                        {
                            if(t == q || t == r || t == s || t == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                            {
                                continue;
                            }
                            tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(t - 2);//假设第四个块的内容.
                            strncpy(guessContent + 4*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            for (int u = 2; u < 12; u++)
                            {
                                if(u == q || u == r || u == s || u ==t || u == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                {
                                    continue;
                                }
                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(u - 2);//假设第四个块的内容.
                                strncpy(guessContent + 5*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                for (int v = 2; v < 12; v++)
                                {
                                    if(v == q || v == r || v == s || v == t || v == u || v == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                    {
                                        continue;
                                    }
                                    tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(v - 2);//假设第四个块的内容.
                                    strncpy(guessContent + 6*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster,be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                    for (int w = 2; w < 12; w++)
                                    {
                                        if(w == q || w == r || w == s || w == t || w == u || w == v || w == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                        {
                                            continue;
                                        }
                                        tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(w - 2);//假设第四个块的内容.
                                        strncpy(guessContent + 7*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                        for(int x = 2; x < 12; x++)
                                        {
                                            if(x == q || x == r || x == s || x == t || x == u || x == v || x == w || x == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                            {
                                                continue;
                                            }
                                            tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(x - 2);//假设第四个块的内容.
                                            strncpy(guessContent + 8*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                            for (int y = 2; y < 12; y++)
                                            {
                                                if(y == q || y == r || y == s || y == t || y == u || y == v || y == w || y == x || y == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                                {
                                                    continue;
                                                }
                                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(y - 2);//假设第四个块的内容.
                                                strncpy(guessContent + 9*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-9*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                                //改动内容：numOfCluster更改，上一篇的结尾strncpysize改回512 新建for循环放进去， 添加一个continue条件，, tempcluster 内容更改，strncpy+1:size更改。
                                                SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                                                if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                                                {
                                                    a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                                                    a[1] = q;
                                                    a[2] = r;
                                                    a[3] = s;
                                                    a[4] = t;
                                                    a[5] = u;
                                                    a[6] = v;
                                                    a[7] = w;
                                                    a[8] = x;
                                                    a[9] = y;
                                                    order = 10;
                                                    return 0;//代表文件符合要求
                                                }
                                            }
                                        } 
                                    }                                   
                                }
                            }
                        }
                    }
                }
            }
            else if (numOfCluster == 11)
            {
                tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(q - 2);//假设第二个块的内容.
                strncpy(guessContent + be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                for(int r = 2; r < 12; r++)
                {
                    if(r == q || r == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                    {
                        continue;
                    }
                    tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(r - 2);//假设第三个块的内容.
                    strncpy(guessContent + 2*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                    for (int s = 2; s < 12; s++)
                    {
                        if(s == q || s == r || s == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                        {
                            continue;
                        }
                        tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(s - 2);//假设第四个块的内容.
                        strncpy(guessContent + 3*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                        for (int t = 2; t < 12; t++)
                        {
                            if(t == q || t == r || t == s || t == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                            {
                                continue;
                            }
                            tempCluster = (char*)addrdata +  (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(t - 2);//假设第四个块的内容.
                            strncpy(guessContent + 4*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                            for (int u = 2; u < 12; u++)
                            {
                                if(u == q || u == r || u == s || u ==t || u == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                {
                                    continue;
                                }
                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(u - 2);//假设第四个块的内容.
                                strncpy(guessContent + 5*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                for (int v = 2; v < 12; v++)
                                {
                                    if(v == q || v == r || v == s || v == t || v == u || v == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                    {
                                        continue;
                                    }
                                    tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(v - 2);//假设第四个块的内容.
                                    strncpy(guessContent + 6*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster,be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                    for (int w = 2; w < 12; w++)
                                    {
                                        if(w == q || w == r || w == s || w == t || w == u || w == v || w == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                        {
                                            continue;
                                        }
                                        tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(w - 2);//假设第四个块的内容.
                                        strncpy(guessContent + 7*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                        for(int x = 2; x < 12; x++)
                                        {
                                            if(x == q || x == r || x == s || x == t || x == u || x == v || x == w || x == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                            {
                                                continue;
                                            }
                                            tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(x - 2);//假设第四个块的内容.
                                            strncpy(guessContent + 8*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                            for (int y = 2; y < 12; y++)
                                            {
                                                if(y == q || y == r || y == s || y == t || y == u || y == v || y == w || y == x || y == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                                {
                                                    continue;
                                                }
                                                tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(y - 2);//假设第四个块的内容.
                                                strncpy(guessContent + 9*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                                //改动内容：numOfCluster更改，上一篇的结尾strncpysize改回512 新建for循环放进去， 添加一个continue条件，, tempcluster 内容更改，strncpy+1:size更改。
                                                for (int z = 2; z < 12; z++)
                                                {
                                                    if(z == q || z == r || z == s || z == t || z == u || z == v || z == w || z == x || z == y || z == de->DIR_FstClusHI*factor + de->DIR_FstClusLO)
                                                    {
                                                        continue;
                                                    }
                                                    tempCluster = (char*)addrdata + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(z - 2);//假设第四个块的内容.
                                                    strncpy(guessContent + 10*be->BPB_BytsPerSec * be->BPB_SecPerClus, tempCluster, de->DIR_FileSize-10*be->BPB_BytsPerSec * be->BPB_SecPerClus);
                                                    SHA1((unsigned char*)guessContent, de->DIR_FileSize, obuf);
                                                    if (strncmp((char*)obuf, sha, SHA_DIGEST_LENGTH) == 0)
                                                    {
                                                        a[0] = de->DIR_FstClusHI*factor + de->DIR_FstClusLO;
                                                        a[1] = q;
                                                        a[2] = r;
                                                        a[3] = s;
                                                        a[4] = t;
                                                        a[5] = u;
                                                        a[6] = v;
                                                        a[7] = w;
                                                        a[8] = x;
                                                        a[9] = y;
                                                        a[10] = z;
                                                        order = 11;
                                                        return 0;//代表文件符合要求
                                                    }
                                                }                                              
                                            }
                                        } 
                                    }                                   
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return -1;
}

int Commandr(char *arg, char* recfilename)
{
    char *addr;
    int fd;
    struct stat buf;//sys/stat

    fd = open(arg, O_RDWR);
    if ( fd < 0) {
        return -1;
    }
    fstat(fd, &buf);//&取地址
    int filesize = buf.st_size;//取文件大小
    addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    int count = 0;
    int countdel = 0;
    int flag = 0;

    BootEntry * be = (BootEntry*) addr;
    
    char *addrdata = ((char*)addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt) + (be->BPB_NumFATs * be->BPB_FATSz32 * be->BPB_BytsPerSec); //每一跳是一个字节跳过了boot和fat部分
    DirEntry * de = (struct DirEntry*)(addrdata+be->BPB_SecPerClus*be->BPB_BytsPerSec*(be->BPB_RootClus-2));
    
    int * addrfat = (int*) (((char* )addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt));
    DirEntry * deflag;

    if(*(addrfat+be->BPB_RootClus)>= 0x0ffffff8)//如果cluster2末尾是EOF
    {
        while (de->DIR_Name[0] != 0)//存在或者已经删除
        {
            if (de->DIR_Name[0] == 0xe5){//如果是删除的文件的话，
                int ret = Check(de, recfilename);
                if (ret == 0)
                {
                    flag++;
                    deflag = de;
                }
                de++;
                countdel++;
                continue;
            }
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){//这里原本是16
 
                break;
            }
            de++;
        }

    }
    //第一个块是cluster2(11/5),56个块是cluster57(11,5), 57(11)
    else //这里是如果包括16个文件以上。
    { 
        int j = be->BPB_RootClus;
        while ( *(addrfat+j) < 0x0ffffff8){//addrfat一直没有变化
        // int a = 00001; a 是一个8进制数.
        //j =2, 56th cluster;
            for(int i = 0; i < be->BPB_BytsPerSec * be->BPB_SecPerClus / 32; i++)//这里16改回来成这个
            {
                if (de->DIR_Name[0] == 0xe5)//de要处理一下偏移量
                {
                    int ret = Check(de, recfilename);
                    if (ret == 0)
                    {
                        flag++;
                        deflag = de;
                    }
                    de++;
                    continue;
                }
                de++;
            }
            de = (DirEntry*) (((char*)addrdata) + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(*(addrfat+j)-2));//(char*)无所谓的
            j = *(addrfat +j);//换新区
        }

        while (de->DIR_Name[0] != 0){//需要一个sum变量来计算，防止bug。
            if (de->DIR_Name[0] == 0xe5)
            {
                int ret = Check(de, recfilename);
                if (ret == 0)
                {
                    flag++;
                    deflag = de;
                }
                de++;
                countdel++;
                continue;
            }
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){
                break;
            }
            de++;
        }

        
        
    }
    if (flag == 0)
    {
        printf("%s: file not found\n", recfilename);
    }
    else if (flag == 1)
    {
        Recover(addrfat, deflag, be, recfilename);//这里要改？
        printf("%s: successfully recovered\n", recfilename);
    }
    else
    {
        printf("%s: multiple candidates found\n", recfilename);
    }
    munmap(addr, filesize);
    close(fd);
    return 0;
}

int CommandSha(char *arg, char* recfilename, char* sha)
{
    char *addr;
    int fd;
    struct stat buf;//sys/stat

    fd = open(arg, O_RDWR);
    if ( fd < 0) {
        return -1;
    }
    fstat(fd, &buf);//&取地址
    int filesize = buf.st_size;//取文件大小
    addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    int count = 0;
    int countdel = 0;
    int flag = 0;

    BootEntry * be = (BootEntry*) addr;

    char *addrdata = ((char*)addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt) + (be->BPB_NumFATs * be->BPB_FATSz32 * be->BPB_BytsPerSec); //每一跳是一个字节跳过了boot和fat部分
    DirEntry * de = (struct DirEntry*)(addrdata+be->BPB_SecPerClus*be->BPB_BytsPerSec*(be->BPB_RootClus-2));
    
    int * addrfat = (int*) (((char* )addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt));
    DirEntry * deflag;


    if(*(addrfat+be->BPB_RootClus)>= 0x0ffffff8)//如果cluster2末尾是EOF
    {
        while (de->DIR_Name[0] != 0)//存在或者已经删除
        {
            if (de->DIR_Name[0] == 0xe5){//如果是删除的文件的话，
                int ret = CheckSha(addrfat, de, be, recfilename, sha);
                if (ret == 0)
                {
                    flag++;
                    deflag = de;
                }
                de++;
                countdel++;
                continue;
            }
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){//这里原本是16
 
                break;
            }
            de++;
        }

    }
    //第一个块是cluster2(11/5),56个块是cluster57(11,5), 57(11)
    else //这里是如果包括16个文件以上。
    { 
        int j = be->BPB_RootClus;
        while ( *(addrfat+j) < 0x0ffffff8){//addrfat一直没有变化
        // int a = 00001; a 是一个8进制数.
        //j =2, 56th cluster;
            for(int i = 0; i < be->BPB_BytsPerSec * be->BPB_SecPerClus / 32; i++)//这里16改回来成这个
            {
                if (de->DIR_Name[0] == 0xe5)//de要处理一下偏移量
                {
                    int ret = CheckSha(addrfat, de, be, recfilename, sha);
                    if (ret == 0)
                    {
                        flag++;
                        deflag = de;
                    }
                    de++;
                    continue;
                }
                de++;
            }
            de = (DirEntry*) (((char*)addrdata) + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(*(addrfat+j)-2));//(char*)无所谓的
            j = *(addrfat +j);//换新区
        }

        while (de->DIR_Name[0] != 0){//需要一个sum变量来计算，防止bug。
            if (de->DIR_Name[0] == 0xe5)
            {
                int ret = CheckSha(addrfat, de, be, recfilename, sha);
                if (ret == 0)
                {
                    flag++;
                    deflag = de;
                }
                de++;
                countdel++;
                continue;
            }
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){
                break;
            }
            de++;
        }

        
        
    }
    if (flag == 0)
    {
        printf("%s: file not found\n", recfilename);
    }
    else if (flag == 1)
    {
        Recover(addrfat, deflag, be, recfilename);//这里要改？
        printf("%s: successfully recovered with SHA-1\n", recfilename);
    }
    else
    {
        printf("%s: multiple candidates found\n", recfilename);
    }
    munmap(addr, filesize);
    close(fd);
    
    return 0;
}

int Commandl(char *arg)
{
    char *addr;
    int fd;
    struct stat buf;//sys/stat

    int count = 0;
    int countdel = 0;

    fd = open(arg, O_RDWR);
    if ( fd < 0) {
        return -1;
    }
    fstat(fd, &buf);//&取地址
    int filesize = buf.st_size;//取文件大小
    addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    BootEntry * be = (BootEntry*) addr;

    //BPB_BytsPerSec; 
    //BPB_RsvdSecCnt;
    char *addrdata = ((char*)addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt) + (be->BPB_NumFATs * be->BPB_FATSz32 * be->BPB_BytsPerSec); //每一跳是一个字节跳过了boot和fat部分
    DirEntry * de = (struct DirEntry*)(addrdata+be->BPB_SecPerClus*be->BPB_BytsPerSec*(be->BPB_RootClus-2));
    
    int * addrfat = (int*) (((char* )addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt));

    int sum =0;//定义一个sum用来保存目前已经计算完的可用有效文件。
    if(*(addrfat+be->BPB_RootClus)>= 0x0ffffff8)//如果cluster2末尾是EOF
    {
        while (de->DIR_Name[0] != 0){//存在或者已经删除
            if (de->DIR_Name[0] == 0xe5){//如果是删除的文件的话，
                de++;
                countdel++;
                continue;
            }
            printL(de);
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){//这里原本是16
                break;
            }
            de++;
        }
        sum = count; 
    }
    //第一个块是cluster2(11/5),56个块是cluster57(11,5), 57(11)
    else //这里是如果包括16个文件以上。
    { 
        int j = be->BPB_RootClus;
        while ( *(addrfat+j) < 0x0ffffff8){//addrfat一直没有变化
        // int a = 00001; a 是一个8进制数.
        //j =2, 56th cluster;
            for(int i = 0; i < be->BPB_BytsPerSec * be->BPB_SecPerClus / 32; i++)//这里16改回来成这个
            {
                if (de->DIR_Name[0] == 0xe5){//de要处理一下偏移量
                    de++;
                    continue;
                }
                printL(de);
                count++;
                de++;
            }
            de = (DirEntry*) (((char*)addrdata) + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(*(addrfat+j)-2));//(char*)无所谓的
            sum += count;//保存起来
            count = 0;
            j = *(addrfat +j);//换新区
        }

        while (de->DIR_Name[0] != 0){//需要一个sum变量来计算，防止bug。
            if (de->DIR_Name[0] == 0xe5){
                de++;
                countdel++;
                continue;
            }
            printL(de);
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){
                break;
            }
            de++;
        }
        sum += count;
        count = 0;//没有什么必要。
    }
    printf("Total number of entries = %d\n", sum);
    munmap(addr, filesize);
    close(fd);
    return 0;//void 函数最后一行的话，return其实没有必要
}

int CommandR(char *arg, char* recfilename, char* sha)
{
    char *addr;
    int fd;
    struct stat buf;//sys/stat

    fd = open(arg, O_RDWR);
    if ( fd < 0) {
        return -1;
    }
    fstat(fd, &buf);//&取地址
    int filesize = buf.st_size;//取文件大小
    addr = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    int count = 0;
    int countdel = 0;
    int flag = 0;

    BootEntry * be = (BootEntry*) addr;

    char *addrdata = ((char*)addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt) + (be->BPB_NumFATs * be->BPB_FATSz32 * be->BPB_BytsPerSec); //每一跳是一个字节跳过了boot和fat部分
    DirEntry * de = (struct DirEntry*)(addrdata+be->BPB_SecPerClus*be->BPB_BytsPerSec*(be->BPB_RootClus-2));
    int * addrfat = (int*) (((char* )addr) + (be->BPB_BytsPerSec * be->BPB_RsvdSecCnt));
    DirEntry * deflag;

    if(*(addrfat+be->BPB_RootClus)>= 0x0ffffff8)//如果cluster2末尾是EOF
    {
        while (de->DIR_Name[0] != 0)//存在或者已经删除
        {
            if (de->DIR_Name[0] == 0xe5){//如果是删除的文件的话，//checksum校验和
                int ret = CheckShaForce(addrfat, de, be, recfilename, sha);
                if (ret == 0)
                {
                    flag++;
                    deflag = de;
                }
                de++;
                countdel++;
                continue;
            }
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){//这里原本是16
 
                break;
            }
            de++;
        }

    }
    //第一个块是cluster2(11/5),56个块是cluster57(11,5), 57(11)
    else //这里是如果包括16个文件以上。
    { 
        int j = be->BPB_RootClus;
        while ( *(addrfat+j) < 0x0ffffff8){//addrfat一直没有变化
        // int a = 00001; a 是一个8进制数.
        //j =2, 56th cluster;
            for(int i = 0; i < be->BPB_BytsPerSec * be->BPB_SecPerClus / 32; i++)//这里16改回来成这个
            {
                if (de->DIR_Name[0] == 0xe5)//de要处理一下偏移量
                {
                    int ret = CheckShaForce(addrfat, de, be, recfilename, sha);
                    if (ret == 0)
                    {
                        flag++;
                        deflag = de;
                    }
                    de++;
                    continue;
                }
                de++;
            }
            de = (DirEntry*) (((char*)addrdata) + (be->BPB_BytsPerSec * be->BPB_SecPerClus)*(*(addrfat+j)-2));//(char*)无所谓的
            j = *(addrfat +j);//换新区
        }

        while (de->DIR_Name[0] != 0){//需要一个sum变量来计算，防止bug。
            if (de->DIR_Name[0] == 0xe5)
            {
                int ret = CheckShaForce(addrfat, de, be, recfilename, sha);
                if (ret == 0)
                {
                    flag++;
                    deflag = de;
                }
                de++;
                countdel++;
                continue;
            }
            count++;
            if (count + countdel == be->BPB_BytsPerSec * be->BPB_SecPerClus / 32){
                break;
            }
            de++;
        }
        
    }
    if (flag == 0)
    {
        printf("%s: file not found\n", recfilename);
    }
    else if (flag == 1)
    {
        RecoverForce(addrfat, deflag, be, recfilename);//这里要改？
        printf("%s: successfully recovered with SHA-1\n", recfilename);
    }
    munmap(addr, filesize);
    close(fd);
    return 0;
}

void printU()
{
    printf("Usage: ./nyufile disk <options>\n");
    printf("  -i                     Print the file system information.\n");
    printf("  -l                     List the root directory.\n");
    printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
    printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
}

int HexToBin(char ch) {
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' +10;
    }
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    return -1;
}
// int commandHandler(char * args[]){
// //if(strcmp(args[0],"-i") == 0) exit(0);
// }
//All information you need is in the boot sector
//如何读取boot sector？
int main(int argc, char *argv[]) {

    //for之前先判断argc，如果等于1就是没给参数，所以直接报错flag =0
    //print usage 可以搞成一个function
    //for (i=0; i < argc; i++) {

    if (argc < 3 || argc > 6){
        printU();
        exit(-1);
    }
        
    if (strcmp(argv[2], "-i") == 0 && argc == 3) {
        int ret = Commandi(argv[1]);
        if ( ret == -1) {
            //printf("You enter a wrong file name.\n");
            printU();
            return -1;
        } else {
            return 0;
        }
    }
    //echo $?这个是用来检测上一条语句的返回值

    if (strcmp(argv[2], "-l") == 0  && argc == 3) {
        int ret = Commandl(argv[1]);//这样的话会更严谨一些
        if ( ret == -1) {
            //printf("You enter a wrong file name.\n");
            printU();
            return -1;
        } else {
            return 0;
        }
    }

    if (strcmp(argv[2], "-r") == 0) {
        if (argc == 4 ){
            Commandr(argv[1], argv[3]);
            return 0;
        }
        else if (argc == 6 && strcmp(argv[4], "-s") == 0){
            char sha1[20];
            for(int i = 0; i < 20; i++)
            {
                sha1[i] = (char) (HexToBin(argv[5][2*i]) * 16 + HexToBin(argv[5][2*i + 1]));
            }
            CommandSha(argv[1], argv[3], sha1);
            return 0;
        }
        else{
            printU(); 
            exit(-1);
        }       
    }

    if (strcmp(argv[2], "-R") == 0 && argc == 6 && strcmp(argv[4], "-s") == 0) 
    {        
        char sha1[20];
        for(int i = 0; i < 20; i++)
        {
            sha1[i] = (char) (HexToBin(argv[5][2*i]) * 16 + HexToBin(argv[5][2*i + 1]));
        }
        CommandR(argv[1], argv[3], sha1);
        return 0;
    }

    printU();

    return 0;
}

//问题：55 行和43行有什么区别？
//结构体是不是可以省略？
//hexdump如何理解： 备份？改成人类可以读的格式
//编译后开局输入是否可以？会不会有autograder？
//编译成功后运行，为什么会出现乱码？
//趁寒假学java;
//head first java;

//改main函数里面的open相关，统一放到main函数开头。
//后缀名只有1个或者两个？后缀名要么是0个？hello 8.3
//具体实现方法？空文件的意义？

//1130:改check函数，第一步存起来遍历de是没有改动的，存贮变量遇到多少个文件,if -》0notfound 1：恢复， 》=2 报错
//7a和7b有没有区别：指针跨度理论上都是一样的？有需要care的地方吗？since continues. 

//8.第一种只有三块：第一种for 2， 3 的时候？
//第二种是3块所有的可能性，在优化？3^3