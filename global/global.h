#include <stdio.h>

#include "var.h"
#ifndef __GLOBAL__
#define __GLOBAL__
extern char sysname[];
extern char pwd[];
extern FILE* DISK;
extern BLOCKZERO blockZero;
extern FCB presentFCB;
extern FATitem FAT1[];
extern FATitem FAT2[];
extern useropen uopenlist[];
#endif
