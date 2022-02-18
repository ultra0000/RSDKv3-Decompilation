#ifndef __FILEBROWSER__
#define __FILEBROWSER__


typedef struct
{
	char** files;
	int    fileNum;

	char** directories;
	int    dirNum;

	char* currentDir;
} dirList_t;

int getDir(dirList_t* dirList);

int printDir(int from, int max, int select, dirList_t dirList);

#endif
