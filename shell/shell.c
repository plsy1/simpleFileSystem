#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../function/function.h"
#include "../global/global.h"
#include "../util/str.h"
char* header() {
  char* buff;
  buff = (char*)malloc(sizeof(char) * 100);
  sprintf(buff, "\033[33m\033[01m%s\033[0m:\033[36m\033[01m%s\033[0m$ ",
          sysname, pwd);
  return buff;
}

char** getInstruction(int* argc) {
  char* buff;
  char** ins;
  buff = (char*)malloc(sizeof(char) * 100);
  ins = (char**)malloc(sizeof(char*) * 10);
  for (int i = 0; i != 10; ++i) {
    ins[i] = (char*)malloc(sizeof(char) * 10);
  }
  printf("%s", header());
  fgets(buff, 100, stdin);
  buff[strlen(buff) - 1] = '\0';
  buff = trim(buff);
  ins = split(buff, " ", argc);
  return ins;
}

void help() {
  printf("-------------------------帮助-------------------------\n");
  printf("%-10s - %s\n", "touch", "创建文件");
  printf("%-10s - %s\n", "rm", "删除文件");
  printf("%-10s - %s\n", "open", "打开文件");
  printf("%-10s - %s\n", "close", "关闭文件");
  printf("%-10s - %s\n", "write", "写数据到文件");
  printf("%-10s - %s\n", "read", "查看文件数据");
  printf("%-10s - %s\n", "pwd", "显示当前目录");
  printf("%-10s - %s\n", "ls", "输出当前目录列表");
  printf("%-10s - %s\n", "mkdir", "创建文件夹");
  printf("%-10s - %s\n", "rmdir", "删除文件夹");
  printf("%-10s - %s\n", "cd", "切换工作目录");
  printf("%-10s - %s\n", "exit", "退出");
}

int doOperation(int argc, char** argv) {
  if (strcmp(argv[0], "exit") == 0) {
    exitsys();
    return -2;
  }
  if (strcmp(argv[0], "help") == 0) {
    if (argc > 1) {
      printf("%s : too many arguments\n", argv[0]);
      return -1;
    } else {
      help();
      return 0;
    }
  }
  if (strcmp(argv[0], "touch") == 0) {
    if (argc != 2) {
      printf("usage %s [file name]\n", argv[0]);
      return -1;
    } else {
      createFile(argv[1]);
      return 0;
    }
  }
  if (strcmp(argv[0], "ls") == 0) {
    if (argc > 1) {
      printf("too many arguments.\n");
      return -1;
    } else {
      ls_();
      return 0;
    }
  }

  if (strcmp(argv[0], "rm") == 0) {
    if (argc != 2) {
      printf("usage %s [file name]\n", argv[0]);
      return -1;
    } else {
      rm_(argv[1]);
      return 0;
    }
  }

  if (strcmp(argv[0], "open") == 0) {
    if (argc != 2) {
      printf("usage %s [file name]\n", argv[0]);
      return -1;
    } else {
      openFile(argv[1]);
      return 0;
    }
  }

  if (strcmp(argv[0], "write") == 0) {
    if (argc != 2) {
      printf("usage %s [fd]\n", argv[0]);
      return -1;
    }
    int a, len = 0;
    a = atoi(argv[1]);
    if (writeTo(a, &len) == 0) {
      printf("write succeed.\n");
      return 0;
    }
    return 0;
  }

  if (strcmp(argv[0], "read") == 0) {
    if (argc != 2) {
      printf("usage %s [fd num]\n", argv[0]);
      return -1;
    } else {
      int a, len = 0;
      a = atoi(argv[1]);
      if (strcmp(argv[1], "0") && a == 0) {
        printf("usage %s [fd num]\n", argv[0]);
        return -1;
      }
      if (readFrom(a, &len) == 0) printf("read fd %d with %d bytes\n", a, len);
      return 0;
    }
  }

  if (strcmp(argv[0], "pwd") == 0) {
    if (argc > 1) {
      printf("too many arguments\n");
      return -1;
    } else {
      printf("%s\n", getPwd());
      return 0;
    }
  }

  if (strcmp(argv[0], "mkdir") == 0) {
    if (argc != 2) {
      printf("usage %s [directory name]\n", argv[0]);
      return -1;
    } else {
      createDir(argv[1]);
      return 0;
    }
  }

  if (strcmp(argv[0], "cd") == 0) {
    if (argc != 2) {
      printf("usage %s [directory name]\n", argv[0]);
      return -1;
    } else {
      cd_(argv[1]);
      return 0;
    }
  }

  if (strcmp(argv[0], "rmdir") == 0) {
    if (argc != 2) {
      printf("usage %s [directory name]\n", argv[0]);
      return -1;
    } else {
      deleteDir(argv[1]);
      return 0;
    }
  }

  if (strcmp(argv[0], "close") == 0) {
    if (argc != 2) {
      printf("usage %s [fd num]\n", argv[0]);
      return -1;
    } else {
      int a;
      a = atoi(argv[1]);
      if (strcmp(argv[1], "0") && a == 0) {
        printf("usage %s [fd num]\n", argv[0]);
        return -1;
      }
      closeFile(a);
      return 0;
    }
  }

  printf("%s : command not found\n", argv[0]);
  return 0;
}

void run() {
  char buff[100];
  char** argv;
  int argc, flag;
  argv = (char**)malloc(sizeof(char*) * 10);
  for (int i = 0; i != 10; ++i) {
    argv[i] = (char*)malloc(sizeof(char) * 10);
    printf("System boot successfully, type \" help \" for help\n");
    while (1) {
      argv = getInstruction(&argc);
      flag = doOperation(argc, argv);
      if (flag == -2) break;
    }
    return;
  }
}