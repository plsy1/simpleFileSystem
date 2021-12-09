#ifndef __FUNCTION__
#define __FUNCTION__

void format();
void init();

int createFile(char* filename);
int deleteFile(char* filename);

int createDir(char* dirname);
int deleteDir(char* dirname);

int changeDirectory(char* dirname);
void showList();

int openFile(char* filename);
int closeFile(int fd);

int writeTo(int fd, int* sumlen);
int readFrom(int fd, int* sumlen);

char* getPwd();
void exitsys();
#endif