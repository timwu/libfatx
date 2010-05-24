#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "libfatx.h"
#include "libfatx_internal.h"

fatx_options_t fatx_options = {
			.filePerm = 0555,
};

void test_splitPath(void)
{
	fatx_filename_list * list = fatx_splitPath("/Content");
	printf("path split test: \n");
	for(;list != NULL; list = list->next) {
		printf("\tentry = %s\n", list->filename);
	}
	fatx_freeFilenameList(list);
}

void test_initFree(const char * path)
{
	fatx_t fatx;
	fatx = fatx_init(path, &fatx_options);
	if (fatx == NULL) {
		printf("Failed to instatiate fatx.\n");
		return;
	}
	fatx_printInfo(fatx);
	fatx_free(fatx);
}

void test_getFatEntry(fatx_t fatx, const char * path)
{
	printf("Fat entry = 0x%x\n", fatx_readFatEntry(fatx, 0x400));
}
void test_listDir(fatx_t fatx, const char * path)
{
	fatx_dir_iter_t iter = fatx_opendir(fatx, path);
	fatx_dirent_t * dirent;
	if (iter == NULL) 
		printf("\"%s\" is not a folder.\n", path);
	printf("Testing listing directory: %s\n", path);
	while( (dirent = fatx_readdir(iter))) {
		printf("\tdirentry = %s\n", dirent->d_name);
	}
	fatx_closedir(iter);
}

void
test_testStat(fatx_t fatx, const char * path)
{
	struct stat st_buf;
	int ret = fatx_stat(fatx, path, &st_buf);
	if(ret == -ENOENT) {
		printf("\"%s\" does not exist.\n", path);
		return;
	}
	printf("Stat info for %s\n", path);
	if(S_ISDIR(st_buf.st_mode)) printf("\tIs a folder.\n");
	if(S_ISREG(st_buf.st_mode)) printf("\tIs a file.\n");
	printf("\tsize = %u bytes\n", st_buf.st_size);
	printf("\taccess time = %s\n", ctime(&st_buf.st_atime));
	printf("\tmodification time = %s\n", ctime(&st_buf.st_mtime));
}

void
test_read(fatx_t fatx, const char * path, const char * outfile)
{
	struct stat st_buf;
	char * buffer;
	int ret;
	if(fatx_stat(fatx, path, &st_buf) == -ENOENT) {
		printf("\"%s\" does exist.\n", path);
		return;
	}
	buffer = (char *) malloc(st_buf.st_size);
	ret = fatx_read(fatx, path, buffer, st_buf.st_size, st_buf.st_size);
	printf("Filesize is %u\n", st_buf.st_size);
	printf("Read returned %d\n", ret);
	free(buffer);
}

int
main(int argc, char* argv[])
{
	fatx_t fatx;
	fatx_options.user = getuid();
	fatx_options.group = getgid();
	if (argc < 2) {
		printf("Not enough args!\n");
		return -1;
	}
	fatx = fatx_init(argv[1], &fatx_options);
	test_read(fatx, "/Cache/TU_1A581VI_000000K000000.0000000000085", "");
	//test_initFree(argv[1]);
	//test_getFatEntry(argv[1]);
	//test_listDir(fatx, "/Cache");
	test_testStat(fatx, "/Content/E0000211D831B603/FFFE07D1/00010000/E0000211D831B603");
	test_testStat(fatx, "/");
	test_splitPath();
	return 0;
}
