#ifndef __FUNCTION__
#define __FUNCTION__
void format();
void init();
int createFile(char* filename);
int deleteDir(char* dirname);
int createDir(char* dirname);
int cd_(char* dirname);
void exitsys();
void ls_();
int rm_(char* filename);
int openFile(char* filename);
int writeTo(int fd, int* sumlen);
int readFrom(int fd, int* sumlen);
char* getPwd();
int closeFile(int fd);
#endif