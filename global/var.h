#ifndef __VAR__
#define __VAR__
#include <stdio.h>

#include "../util/list.h"
#include "define.h"
typedef struct BLOCKZERO {
  char id[10];
  char info[200];
  unsigned short root;
  int startblock;
  int rootFCB;
} BLOCKZERO;

typedef struct FCB {
  char name[FILE_NAME_LEN];
  unsigned type : 1;
  unsigned use : 1;
  unsigned short time;
  unsigned short date;
  unsigned int base;
  unsigned int len;
} FCB;

typedef struct FCBList {
  FCB fcb_entry;
  lslink link;
} FCBList;

//每个FATitem大小为2B
typedef struct FATitem {
  signed short item : 16;
} FATitem;

typedef struct useropen {
  FCB fcb;
  char dir[80];
  unsigned int count;     //文件指针的位置
  unsigned fcbstate : 1;  //标志fcb是否被修改 1-已修改 0-未修改
  unsigned topenfile : 1;  //标志使用状态 1-已使用(USED) 0-未使用(FREE)
  int blocknum;            //所在块号
  int offset_in_block;     //所在块号偏移量
} useropen;

//盘块链
typedef struct blockchain {
  signed short blocknum : 16;
  lslink link;
} blockchain;
#endif