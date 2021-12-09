#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../global/define.h"
#include "../global/global.h"
#include "../util/disk.h"
//删除文件
int deleteFile(char *filename) {
  //找到要删除文件在其父目录的FCBlist中的下标
  int offset = findFCBInBlockByName(filename, presentFCB.base);
  //下标小于0,即当前目录下不存在这个文件
  if (offset < 0) {
    printf("No such file: %s\n", filename);
    return -1;
  } else {
    FCB fcb;
    //取得要删除文件的FCB
    getFCB(&fcb, presentFCB.base, offset);
    //是个目录，删除失败
    if (fcb.type == 1) {
      printf("Use rmdir to delete directory\n");
      return -1;
    }
    //将要删除文件所占用的块释放
    blockchain *blc;
    lslink *temp;
    blc = getBlockChain(fcb.base);
    list_for_each(temp, &(blc->link)) {
      blockchain *b = list_entry(temp, struct blockchain, link);
      FAT1[b->blocknum].item = FREE;
      FAT2[b->blocknum].item = FREE;
    }
    FAT1[fcb.base].item = FREE;
    FAT2[fcb.base].item = FREE;
    //将FAT写回磁盘
    rewriteFAT();
    //从父目录中删除此文件FCB
    removeFCB(presentFCB.base, offset);
    //如果已打开此文件，则将其从文件打开表中删除
    int fd = findfdByNameAndDir(filename, pwd);
    if (fd >= 0 && uopenlist[fd].topenfile == USED)
      uopenlist[fd].topenfile = FREE;
    return 0;
  }
}

//退出系统
void exitsys() { fclose(DISK); }

//格式化磁盘文件
void format() {
  //初始化引导块
  strcpy(blockZero.id, "FORMAT");  //设置初始化标志
  strcpy(blockZero.info, "blocksize:1024B\nblocknum:1024 Disksize:1MB");
  blockZero.root = ROOT_FCB_LOCATION;
  blockZero.startblock = DATA_INIT_BLOCK;
  blockZero.rootFCB = ROOT_FCB_LOCATION;
  fseek(DISK, 0, SEEK_SET);
  fwrite(&blockZero, sizeof(BLOCKZERO), 1, DISK);

  //初始化FAT表
  FATitem *fat, fi;
  fat = (FATitem *)malloc(sizeof(FATitem) * BLOCK_NUMS * 2);
  memset(fat, 0, sizeof(FATitem));
  fseek(DISK, 1 * BLOCK_SIZE, SEEK_SET);
  fwrite(fat, sizeof(FATitem), BLOCK_NUMS * 2, DISK);
  free(fat);

  //设置根目录
  FCB rootFCB;
  strcpy(rootFCB.name, "/");
  rootFCB.type = 1;    //类型-目录
  rootFCB.use = USED;  //已使用
  // rootFCB.time = getTime(t);
  // rootFCB.date = getDate(t);
  rootFCB.base = 6;  //起始盘块号
  rootFCB.len = 1;   //长度
  FAT1[5].item = FCB_BLOCK;
  FAT2[5].item = FCB_BLOCK;
  addFCB(rootFCB, 5);

  //根目录FCB
  initFCBBlock(6, 6);

  //修改对应FAT表
  fi.item = END_OF_FILE;
  for (int i = 0; i < 5; i++) FAT1[i].item = USED;
  rewriteFAT();
}

//载入已有磁盘，若不存在则创建
void init() {
  int fd;
  // O_CREAT 如果指定文件不存在，則創建這個文件
  // O_EXCL 如果要創建的文件已存在，則返回 -1，並且修改 errno 的值
  // S_IRWXU 文件所有者读写执行权限
  if ((fd = open(sysname, O_CREAT | O_EXCL, S_IRWXU)) < 0) {
    if (errno == EEXIST) {
      // O_RDWR：可读写方式打开
      if ((fd = open(sysname, O_RDWR, S_IRWXU)) < 0) {
        printf("Open failed.");
        exit(-1);
      }
    } else {
      exit(-1);
    }
  } else {
    close(fd);
    printf("未找到磁盘文件，现在进行创建\n");
    createDisk();
    if ((fd = open(sysname, O_RDWR, S_IRWXU)) < 0) {
      printf("Open failed.");
      exit(-1);
    }
  }
  //将fd转换为文件指针后返回
  if ((DISK = fdopen(fd, "r+")) == NULL) {
    exit(-1);
  }
  char buff[10];
  //读出第一个磁盘块
  if (fread(&blockZero, sizeof(BLOCKZERO), 1, DISK) == 1) {
    //格式化
    if (strcmp(blockZero.id, "FORMAT") != 0) {
      printf("开始格式化磁盘\n");
      format();
    }
  }
  strcpy(pwd, "/");            //设置当前目录为根目录
  getFCB(&presentFCB, 5, 0);   //将根目录设置成当前内存中的FCB
  getFAT(FAT1, FAT1_LOCATON);  //加载FAT1
  getFAT(FAT2, FAT2_LOCATON);  //加载FAT2
                               //初始化文件打开表
  for (int i = 0; i < MAX_FD_NUM; i++)
    uopenlist[i].topenfile = FREE;  //全部设置成0
  return;
}

//创建文件
int createFile(char *filename) {
  if (strlen(filename) > FILE_NAME_LEN) {
    printf("Create failed, File name is too long\n");
    return -1;
  }
  int offset = findFCBInBlockByName(filename, presentFCB.base);
  if (offset > 0) {
    printf("File has been exist\n");
    return -1;
  } else {
    offset = getEmptyFCBOffset(presentFCB.base);
    if (offset < 0) {
      printf("Create failed, directory space is full\n");
      return -1;
    } else {
      int blocknum = getEmptyBlockId();
      if (blocknum < 0) {
        printf("Create failed, disk space is full\n");
        return -1;
      } else {
        FCB fcb;
        strcpy(fcb.name, filename);
        fcb.type = 0;
        fcb.use = USED;
        fcb.base = blocknum;
        fcb.len = 1;
        addFCB(fcb, presentFCB.base);

        FAT1[blocknum].item = END_OF_FILE;
        FAT2[blocknum].item = END_OF_FILE;
        rewriteFAT();
        return 0;
      }
    }
  }
  return 0;
}

//列出当前目录下的文件
void showList() {
  char *type[2] = {"file", "directory"};
  int blocknum = presentFCB.base;
  lslink *FCBListhead, *temp;
  FCBList FL, *Fnode;
  FCBListhead = &(FL.link);
  printf("directory %s\n", pwd);
  printf("%-12s %-10s %-6s\n", "name", "type", "length(bytes)");
  getFCBList(blocknum, FL, FCBListhead);
  //遍历取得的FCBLIST，依次输出
  list_for_each(temp, FCBListhead) {
    Fnode = list_entry(temp, FCBList, link);
    printf("%-12s %-10s %-6d\n", Fnode->fcb_entry.name,
           type[Fnode->fcb_entry.type], Fnode->fcb_entry.len);
  }
}

//将文件添加到文件打开表
int openFile(char *filename) {
  int fd;
  fd = findfdByNameAndDir(filename, pwd);
  if (fd >= 0 && uopenlist[fd].topenfile == USED) {  //判断是否已经打开
    printf("open: cannot open file ‘%s’: %s is already open\n", filename,
           filename);
    return -1;
  }
  if ((fd = getEmptyfd()) < 0) {  //查看是否有空的fd
    printf("open: cannot open file ‘%s’: Lack of empty fd\n", filename);
    return -1;
  } else {
    int offset = findFCBInBlockByName(filename, presentFCB.base);  //获得fcb位置
    if (offset < 0) {
      printf("open: %s: No such file or directory\n", filename);
      return -1;
    } else {
      //构造打开表项
      FCB fcb;
      getFCB(&fcb, presentFCB.base, offset);
      uopenlist[fd].fcb = fcb;
      strcpy(uopenlist[fd].dir, pwd);
      uopenlist[fd].count = BLOCK_SIZE * fcb.base + 0;
      uopenlist[fd].fcbstate = 0;
      uopenlist[fd].topenfile = USED;
      uopenlist[fd].blocknum = presentFCB.base;
      uopenlist[fd].offset_in_block =
          findFCBInBlockByName(filename, presentFCB.base);
      printf("filename:%s fd:%d\n", filename, fd);
      return 0;
    }
  }
}

//往文件写数据
int writeTo(int fd, int *sumlen) {
  if (fd >= MAX_FD_NUM || fd < 0) {  //判断fd合法性
    printf("close: invalid fd\n");
    return -1;
  } else {
    if (uopenlist[fd].topenfile == FREE) {  //判断文件是否已经打开
      printf("write: cannot write to fd ‘%d’: fd %d is already close\n", fd,
             fd);
      return -1;
    } else {
      if (uopenlist[fd].fcb.type == 1) {  //判断如果是目录
        printf("write: cannot write to fd ‘%d’: fd %d is a directory\n", fd,
               fd);
        return -1;
      }
      char str[BLOCK_SIZE], buff[BLOCK_SIZE];
      int blocknum, nextblocknum;
      int len, bloffset;  // len-一次读取的长度 bloffset-文件指针块内偏移量
      *sumlen = 0;  // sumlen-总长(所有输入长度之和)
      memset(str, 0, BLOCK_SIZE);
      memset(buff, 0, BLOCK_SIZE);

      bloffset = 0;
      blocknum = uopenlist[fd].fcb.base;
      //做截断处理
      if (FAT1[blocknum].item != END_OF_FILE) {
        blockchain *blc = getBlockChain(blocknum);
        lslink *temp;
        list_for_each(temp, &(blc->link)) {
          blockchain *b = list_entry(temp, struct blockchain, link);
          //清空对应FAT块
          FAT1[b->blocknum].item = FREE;
          FAT2[b->blocknum].item = FREE;
          uopenlist[fd].fcb.len = 0;
        }
      }
      //循环读取直到EOF 每次最多读取一个盘块大小的内容
      //多余部分留在缓冲区作为下次读取
      while (fgets(str, BLOCK_SIZE, stdin) != NULL) {
        len = strlen(str);  //记录实际读取到的长度
        // printf("len %d\n",len);
        if (bloffset + len < BLOCK_SIZE) {
          //文件指针块内偏移量和将要输入的长度之和小于一个盘块--不停地输入到缓冲区中
          *sumlen += len;
          bloffset += len;
          strcat(buff, str);
        } else {
          //长度大于一个盘块
          int lastlen =
              BLOCK_SIZE - bloffset - 1;  //要留出一位给\0作为结尾标志!!!!!!重要
          int leavelen = len - lastlen;  //计算剩下的长度
          strncat(buff, str, lastlen);   //填满上一个块
          writeToDisk(DISK, buff, BLOCK_SIZE, blocknum * BLOCK_SIZE,
                      0);  //先把之前整个盘块的内容存放起来
          memset(buff, '\0',
                 BLOCK_SIZE);  //重新初始化buff 清空原有数据 准备下一次输入
                               //修改总长度和块内指针位置
          *sumlen += len;
          bloffset = leavelen;
          //将剩下部分输入缓冲区
          strcat(buff, str + lastlen);
          //获得一个新的盘块
          nextblocknum = getEmptyBlockId();
          //判断空间是否充足
          if (nextblocknum < 0) {
            printf("write: cannot write to fd ‘%d’: lack of space\n", fd);
            return -1;
          }
          //标记使用状态(重要！！！)
          FAT1[nextblocknum].item = END_OF_FILE;
          FAT2[nextblocknum].item = END_OF_FILE;
          //修改FAT
          FAT1[blocknum].item = nextblocknum;
          FAT2[blocknum].item = nextblocknum;
          blocknum = nextblocknum;  //修改当前块号
        }
      }
      writeToDisk(DISK, buff, BLOCK_SIZE, blocknum * BLOCK_SIZE, 0);
      uopenlist[fd].count = BLOCK_SIZE * blocknum + bloffset;
      uopenlist[fd].fcb.len = *sumlen;
      //修改对应FCB
      changeFCB(uopenlist[fd].fcb, uopenlist[fd].blocknum,
                uopenlist[fd].offset_in_block);
      //设置文件结尾
      FAT1[blocknum].item = END_OF_FILE;
      FAT2[blocknum].item = END_OF_FILE;
      rewriteFAT();
      return 0;
    }
  }
}

//从文件读出数据并打印
int readFrom(int fd, int *sumlen) {
  if (fd >= MAX_FD_NUM || fd < 0) {
    printf("read: invalid fd\n");
    return -1;
  } else {
    if (uopenlist[fd].topenfile == FREE) {  //判断是否已经关闭
      printf("read: cannot read to fd ‘%d’: fd %d is already close\n", fd, fd);
      return -1;
    } else {
      if (uopenlist[fd].fcb.type == 1) {  //判断如果是目录
        printf("read: cannot read to fd ‘%d’: fd %d is a directory\n", fd, fd);
        return -1;
      }
      char buff[BLOCK_SIZE];
      int blocknum = uopenlist[fd].fcb.base;
      while (blocknum != END_OF_FILE) {
        readFromDisk(DISK, buff, BLOCK_SIZE, blocknum * BLOCK_SIZE, 0);
        *sumlen += strlen(buff);
        fputs(buff, stdout);
        blocknum = FAT1[blocknum].item;
      }
      return 0;
    }
  }
}

//返回当前目录路径
char *getPwd() { return pwd; }

//创建目录
int createDir(char *dirname) {
  //判断文件名长度
  if (strlen(dirname) > FILE_NAME_LEN) {
    printf(
        "mkdir: cannot create directory ‘%s’: directory name must less than %d "
        "bytes\n",
        dirname, FILE_NAME_LEN);
    return -1;
  }
  //检查是否重名
  if (findFCBInBlockByName(dirname, presentFCB.base) > 0) {
    printf("mkdir: cannot create directory ‘%s’: File exists\n", dirname);
    return -1;
  }
  //获得当前目录表中空余位置，以便存放FCB
  int offset = getEmptyFCBOffset(presentFCB.base);
  if (offset < 0) {
    printf("mkdir:no empty space to make directory\n");
    return -1;
  }
  //获得空的盘块以便存储新的目录表
  int blocknum = getEmptyBlockId();
  FCB fcb;
  if (blocknum < 0) {
    printf("mkdir:no empty block\n");
    return -1;
  }
  //构造FCB();
  strcpy(fcb.name, dirname);
  fcb.type = 1;
  fcb.use = USED;
  fcb.base = blocknum;
  fcb.len = 1;
  initFCBBlock(blocknum, presentFCB.base);
  addFCB(fcb, presentFCB.base);  //在当前目录表加入这个目录项
  rewriteFAT();
}

//删除目录
int deleteDir(char *dirname) {
  //检查是否有这个目录
  int offset;
  FCB fcb;  //将要删除的FCB
  if ((offset = findFCBInBlockByName(dirname, presentFCB.base)) < 0) {
    printf("rmdir: cannot remove '%s': No such file or directory\n", dirname);
    return -1;
  } else {
    // 清空这个FCB指向的盘块
    getFCB(&fcb, presentFCB.base, offset);
    //判断是否为目录类型
    if (fcb.type != 1) {
      printf("rmdir: cannot remove '%s': Is a file, please use rm\n", dirname);
      return -1;
    }
    //判断是否删除的是 .或 ..目录
    if (strcmp(fcb.name, ".") == 0 || strcmp(fcb.name, "..") == 0) {
      printf("rmdir: refusing to remove '.' or '..'\n");
      return -1;
    }
    //判断这个目录是否为空
    for (int i = 0; i < FCB_ITEM_NUM; i++) {
      FCB f;
      getFCB(&f, fcb.base, i);
      if (f.use == USED) {
        if (strcmp(f.name, ".") != 0 && strcmp(f.name, "..") != 0) {
          printf("rmdir: cannot remove '%s': %s not empty\n", dirname, dirname);
          return -1;
        }
      }
    }
    //修改FAT
    FAT1[fcb.base].item = FREE;
    FAT2[fcb.base].item = FREE;
    rewriteFAT();
    //删除这个FCB
    removeFCB(presentFCB.base, offset);
    return 0;
  }
}

//切换目录
int changeDirectory(char *dirname) {
  int offset;
  if ((offset = findFCBInBlockByName(dirname, presentFCB.base)) < 0) {
    printf("cd: %s: No such file or directory\n", dirname);
    return -1;
  } else {
    FCB fcb;
    getFCB(&fcb, presentFCB.base, offset);
    if (fcb.type == 0) {
      printf("cd: %s: Not a directory\n", dirname);
      return -1;
    }
    presentFCB = fcb;               //修改当前fcb值
    if (strcmp(dirname, ".") == 0)  //当前目录
      ;
    else if (strcmp(dirname, "..") == 0) {  //上一级目录
      if (strcmp(pwd, "/") != 0) {          //不是根目录情况
        char *a = strchr(pwd, '/');   //从左往右第一次出现/的位置
        char *b = strrchr(pwd, '/');  //从右往左第一次出现/的位置
        if (a != b)  //判断是否只有一个/字符 不相等则有多个
          *b = '\0';
        else
          *(b + 1) = '\0';
      }
    } else {  //下一级目录
      if (strcmp(pwd, "/") != 0) strcat(pwd, "/");
      strcat(pwd, dirname);
    }
    return 0;
  }
}

//关闭文件
int closeFile(int fd) {
  if (fd >= MAX_FD_NUM || fd < 0) {
    printf("close: invalid fd\n");
    return -1;
  } else {
    if (uopenlist[fd].topenfile == FREE) {  //判断是否已经关闭
      printf("close: cannot close fd ‘%d’: fd %d is already close\n", fd, fd);
      return -1;
    }
    if (uopenlist[fd].fcbstate == 1)  // fcb被修改了
      changeFCB(uopenlist[fd].fcb, uopenlist[fd].blocknum,
                uopenlist[fd].offset_in_block);
    uopenlist[fd].topenfile = FREE;  //清空文件打开表项
    return 0;
  }
}