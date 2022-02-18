#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "explorer.h"

static int cmpstringp(const void *p1, const void *p2)
{
	return strcasecmp(* (const char **) p1, * (const char **) p2);
}

int getDir(dirList_t* dirList)
{
	DIR				*dp;
	struct dirent	*ep;
	int				fileNum = 0;
	int				dirNum = 0;
	char*			wd = getcwd(NULL, 0);

	if(wd == NULL)
		return 0;

	/* Clear strings */
	for(int i = 0; i < dirList->dirNum; i++)
		free(dirList->directories[i]);

	for(int i = 0; i < dirList->fileNum; i++)
		free(dirList->files[i]);

	free(dirList->currentDir);

	if((dirList->currentDir = strdup(wd)) == NULL)
		puts("Failed to copy cwd name. (Out of memory?)");

	if((dp = opendir(wd)) != NULL)
	{
		while((ep = readdir(dp)) != NULL)
		{
			if(ep->d_type == DT_DIR)
			{
				/* Add more space for another pointer to a dirent struct */
				dirList->directories = realloc(dirList->directories, (dirNum + 1) * sizeof(char*));

				if((dirList->directories[dirNum] = strdup(ep->d_name)) == NULL)
					puts("Failed to copy dir entry. (Out of memory?)");

				dirNum++;
				continue;
			}

			/* Add more space for another pointer to a dirent struct */
			dirList->files = realloc(dirList->files, (fileNum + 1) * sizeof(char*));

			if((dirList->files[fileNum] = strdup(ep->d_name)) == NULL)
				puts("Failed to copy file entry. (Out of memory?)");

			fileNum++;
		}

		qsort(&dirList->files[0], fileNum, sizeof(char *), cmpstringp);
		qsort(&dirList->directories[0], dirNum, sizeof(char *), cmpstringp);
	}

	dirList->dirNum = dirNum;
	dirList->fileNum = fileNum;

	closedir(dp);
	free(wd);
	return fileNum + dirNum;
}

int printDir(int from, int max, int select, dirList_t dirList)
{
	int				fileNum = 0;
	int				listed = 0;

	printf("\033[0;0H");
	printf("Dir: %.33s\n", dirList.currentDir);

	if(from == 0)
	{
		printf("\33[2K%c../\n", select == 0 ? '>' : ' ');
		listed++;
		max--;
	}

	while(dirList.fileNum + dirList.dirNum > fileNum)
	{
		fileNum++;

		if(fileNum <= from)
			continue;

		listed++;

		if(dirList.dirNum >= fileNum)
		{
			printf("\33[2K%c\x1b[34;1m%.37s/\x1b[0m\n",
					select == fileNum ? '>' : ' ',
					dirList.directories[fileNum - 1]);

		}

		/* fileNum must be referring to a file instead of a directory. */
		if(dirList.dirNum < fileNum)
		{
			printf("\33[2K%c%.37s\n",
					select == fileNum ? '>' : ' ',
					dirList.files[fileNum - dirList.dirNum - 1]);

		}

		if(fileNum == max + from)
			break;
	}

	return listed;
}
