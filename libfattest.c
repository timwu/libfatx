#include <stdio.h>
#include "libfatx.h"
#include "libfatx_internal.h"

void test_splitPath(void)
{
	fatx_filename_list * list = fatx_splitPath("/hello/");
	printf("path split test: \n");
	for(;list != NULL; list = list->next) {
		printf("\tentry = %s\n", list->filename);
	}
	fatx_freeFilenameList(list);
}

void test_initFree(const char * path)
{
	fatx_t fatx;
	fatx = fatx_init(path);
	if (fatx == NULL) {
		printf("Failed to instatiate fatx.\n");
		return -1;
	}
	fatx_printInfo(fatx);
	fatx_free(fatx);
}

void test_getFatEntry(const char * path)
{
	fatx_t fatx;
	fatx = fatx_init(path);
	printf("Fat entry = 0x%x\n", fatx_readFatEntry(fatx, 0x400));
}
void test_listRootDir(const char * path)
{
	fatx_t fatx;
	fatx = fatx_init(path);
	fatx_dir_iter_t iter = fatx_opendir(fatx, "/");
	fatx_dirent_t * dirent;
	printf("Testing listing a directory...\n");
	while( (dirent = fatx_readdir(iter))) {
		printf("\tdirentry = %s\n", dirent->d_name);
	}
	fatx_closedir(iter);
}

int
main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Not enough args!\n");
		return -1;
	}
	//test_initFree(argv[1]);
	//test_getFatEntry(argv[1]);
	test_listRootDir(argv[1]);
	test_splitPath();
	return 0;
}
