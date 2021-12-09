#include <stdio.h>

#include "function/function.h"
#include "global/var.h"
#include "shell/shell.h"
char sysname[20] = "simpleFileSystem";
char pwd[80];
FILE* DISK;
BLOCKZERO blockZero;
FATitem FAT1[FAT_ITEM_NUM], FAT2[FAT_ITEM_NUM];
FCB presentFCB;
useropen uopenlist[MAX_FD_NUM];
int main() {
  init();
  run();
  return 0;
}