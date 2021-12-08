#include "disk.h"

#include <stdio.h>
#include <string.h>

#include "../global/global.h"
#include "list.h"

//创建磁盘文件
void createDisk() {
  char buff[BLOCK_SIZE] = {'0'};
  FILE *f = fopen(sysname, "r+");
  for (int i = 0; i < BLOCK_NUMS; i++) fwrite(buff, sizeof(buff), 1, f);
  fclose(f);
}

//将ptr所指向的内容从base + offset处起连续size字节，写到DISK文件中
int writeToDisk(FILE *DISK, void *ptr, int size, int base, long offset) {
  if (base <= 0)
    fseek(DISK, offset, SEEK_SET);
  else
    fseek(DISK, base + offset, SEEK_SET);
  fwrite(ptr, size, 1, DISK);
  return 0;
}

//从DISK文件，base + offset地址单元处，取连续size个字节存入buff中
int readFromDisk(FILE *DISK, void *buff, int size, int base, long offset) {
  if (base <= 0)
    fseek(DISK, offset, SEEK_SET);
  else
    fseek(DISK, base + offset, SEEK_SET);
  fread(buff, size, 1, DISK);
  return 0;
}

//读取FAT表
int getFAT(FATitem *fat, int fat_location) {
  readFromDisk(DISK, fat, FAT_ITEM_SIZE * FAT_ITEM_NUM, fat_location, 0);
  return 0;
}
//修改FAT表
int changeFAT(FATitem *fat, int fat_location) {
  writeToDisk(DISK, fat, FAT_ITEM_SIZE * FAT_ITEM_NUM, fat_location, 0);
  return 0;
}
//重新加载FAT表
void reloadFAT() {
  getFAT(FAT1, FAT1_LOCATON);
  getFAT(FAT2, FAT2_LOCATON);
}
//修改FAT1 FAT2
void rewriteFAT() {
  changeFAT(FAT1, FAT1_LOCATON);
  changeFAT(FAT2, FAT2_LOCATON);
}
//初始化一个目录块 blocknum-将要初始化的块号 parentblocknum-父目录块号
int initFCBBlock(int blocknum, int parentblocknum) {
  //修改FAT
  FAT1[blocknum].item = FCB_BLOCK;
  FAT2[blocknum].item = FCB_BLOCK;
  rewriteFAT();
  //加入.和..目录
  //.目录
  FCB fcb;
  strcpy(fcb.name, ".");
  fcb.type = 1;
  fcb.use = USED;
  // fcb.time = getTime(t);
  // fcb.date = getDate(t);
  fcb.base = blocknum;  //指向当前目录
  fcb.len = 1;
  addFCB(fcb, blocknum);
  //..目录
  strcpy(fcb.name, "..");
  fcb.type = 1;
  fcb.use = USED;
  // fcb.time = getTime(t);
  // fcb.date = getDate(t);
  fcb.base = parentblocknum;  //指向父目录
  fcb.len = 1;
  addFCB(fcb, blocknum);
  return 0;
}
//将fcb信息加入父目录FCBLIST中
int addFCB(FCB fcb, int blocknum) {
  int offset = getEmptyFCBOffset(blocknum);
  if (offset == -1)
    return -1;
  else {
    writeToDisk(DISK, &fcb, sizeof(FCB), blocknum * BLOCK_SIZE,
                offset * FCB_SIZE);
    return 0;
  }
}
//根据名字在当前目录块寻找对应(已被使用的)FCB
int findFCBInBlockByName(char *name, int blocknum) {
  FCB fcb[FCB_ITEM_NUM];
  int offset = -1;
  readFromDisk(DISK, fcb, sizeof(FCB) * FCB_ITEM_NUM, blocknum * BLOCK_SIZE, 0);
  for (int i = 0; i < FCB_ITEM_NUM; i++) {
    if (fcb[i].use == USED) {
      if (strcmp(fcb[i].name, name) == 0) offset = i;
    }
  }
  return offset;
}
//修改FCB
int changeFCB(FCB fcb, int blocknum, int offset_in_block) {
  writeToDisk(DISK, &fcb, sizeof(FCB), blocknum * BLOCK_SIZE,
              offset_in_block * FCB_SIZE);
  return 0;
}
//删除FCB
int removeFCB(int blocknum, int offset_in_block) {
  FCB fcb;
  readFromDisk(DISK, &fcb, sizeof(FCB), blocknum * BLOCK_SIZE,
               offset_in_block * FCB_SIZE);
  fcb.use = 0;
  writeToDisk(DISK, &fcb, sizeof(FCB), blocknum * BLOCK_SIZE,
              offset_in_block * FCB_SIZE);
  return 0;
}

//从父目录的FCB中，取得可用FCB地址
int getEmptyFCBOffset(int blocknum) {
  FCB fcblist[FCB_ITEM_NUM];
  readFromDisk(DISK, fcblist, sizeof(FCB) * FCB_ITEM_NUM, blocknum * BLOCK_SIZE,
               0);
  int i;
  for (i = 0; i < FCB_ITEM_NUM; i++)
    if (fcblist[i].use == FREE) return i;
  if (i == FCB_ITEM_NUM) return -1;
}
//取得FCB 从blocknum + offset处
int getFCB(FCB *fcb, int blocknum, int offset_in_block) {
  readFromDisk(DISK, fcb, sizeof(FCB), blocknum * BLOCK_SIZE,
               offset_in_block * FCB_SIZE);
  return 0;
}

int getFCBList(int blocknum, FCBList FLstruct, lslink *fcblisthead) {
  FCB fcblist[FCB_ITEM_NUM];
  //读当前目录下所有FCB
  readFromDisk(DISK, &fcblist, sizeof(FCB) * FCB_ITEM_NUM,
               blocknum * BLOCK_SIZE, 0);
  //初始化一个链表，并将读取的fcb依次加入
  list_init(fcblisthead, &FLstruct);
  for (int i = 0; i < FCB_ITEM_NUM; i++) {
    if (fcblist[i].use == USED) {
      FCBList *temp = get_node(FCBList);
      temp->fcb_entry = fcblist[i];
      list_insert(fcblisthead, &(temp->link), temp);
    }
  }
}
//取得当前FCB目录下文件数目
int getFCBNum(int blocknum) {
  int num = 0;
  FCB fcblist[FCB_ITEM_NUM];
  readFromDisk(DISK, &fcblist, sizeof(FCB) * FCB_ITEM_NUM,
               blocknum * BLOCK_SIZE, 0);
  for (int i = 0; i < FCB_ITEM_NUM; i++) {
    if (fcblist[i].use == USED) num++;
  }
  return num;
}

//从FAT表中寻找一个空块
int getEmptyBlockId() {
  int flag = 0;
  for (int i = 0; i < FAT_ITEM_NUM; i++) {
    if (FAT1[i].item == FREE && flag == 0) {
      flag = i;
      break;
    }
  }
  if (flag == 0)
    return -1;
  else
    return flag;
}
//取得打开的文件数
int getOpenNum() {
  int num = 0;
  for (int i = 0; i < MAX_FD_NUM; i++)
    if (uopenlist[i].topenfile == USED) num++;
  return num;
}
//取得可用fd号
int getEmptyfd() {
  int fd = -1;
  for (int i = 0; i < MAX_FD_NUM; i++)
    if ((uopenlist[i].topenfile == FREE) && (fd == -1)) {
      fd = i;
      break;
    }
  return fd;
}
//根据文件名/目录名返回fd
int findfdByNameAndDir(char *filename, char *dirname) {
  int fd = -1;
  for (int i = 0; i < MAX_FD_NUM; i++)
    if (strcmp(filename, uopenlist[i].fcb.name) == 0) {
      if (strcmp(dirname, uopenlist[i].dir) == 0) {
        fd = i;
        break;
      }
    }
  return fd;
}

blockchain *getBlockChain(int blocknum) {
  int num = blocknum;
  blockchain *blc, *first;
  blc = get_node(struct blockchain);
  list_init(&(blc->link), blc);
  if (num == 1 || num == 2 || num == -1) {
    first = get_node(struct blockchain);
    first->blocknum = blocknum;
    list_insert(&(blc->link), &(first->link), blc);
    return blc;
  }
  while (num != -1) {
    blockchain *temp;
    temp = get_node(struct blockchain);
    temp->blocknum = num;
    list_insert(&(blc->link), &(temp->link), blc);
    num = FAT1[num].item;
  }
  return blc;
}