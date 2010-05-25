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

void test_splitPath(const char * path)
{
	fatx_filename_list * list = fatx_splitPath(path);
	fatx_filename_list *dirname, *basename;
	printf("full path:\n");
	fatx_printFilenameList(list);
	printf("dirname:\n");
	dirname = fatx_dirname(list);
	fatx_printFilenameList(dirname);
	printf("basename:\n");
	basename = fatx_basename(list);
	fatx_printFilenameList(basename);
	printf("origina:\n");
	fatx_printFilenameList(list);

	//for(;list != NULL; list = list->next) {
		//printf("\tentry = %s\n", list->filename);
	//}
	fatx_freeFilenameList(basename);
	fatx_freeFilenameList(dirname);
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
	ret = fatx_read(fatx, path, buffer, st_buf.st_size -1, st_buf.st_size);
	printf("Filesize is %u\n", st_buf.st_size);
	printf("Read returned %d\n", ret);
	free(buffer);
}

void
test_findFirstFreeDirEntry(fatx_t fatx, const char * path)
{
	fatx_directory_entry * entry = fatx_getFirstOpenDirectoryEntry(fatx, NULL);
	printf("first free filesize = %x", entry->filenameSz);
}

void
test_createFile(fatx_t fatx, const char * path)
{
	int ret = 0;
	ret = fatx_mkfile(fatx, path);
	if(ret != 0) {
		printf("ret = %d\n", ret);
	}
	test_listDir(fatx, "/");
}

void
test_write(fatx_t fatx, const char * path)
{
	int ret = 0;
	char *buf = "abc123";
	char *buf2 = (char *) malloc(7);
	ret = fatx_mkfile(fatx, path);
	if(ret != 0) {
		printf("ret = %d\n", ret);
	}
	ret = fatx_write(fatx, path, buf, 0, 7);
	if(ret != 0) {
		printf("ret = %d\n", ret);
	}
	ret = fatx_read(fatx, path, buf2, 0, 7);
	if(ret != 0) {
		printf("ret = %d\n", ret);
	}
	printf("read data %s\n", buf2);
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
	//test_read(fatx, "/Cache/TU_1A581VI_000000K000000.0000000000085", "");
	//test_initFree(argv[1]);
	//test_getFatEntry(argv[1]);
	//test_listDir(fatx, "/Cache");
	//test_testStat(fatx, "/Content/E0000211D831B603/FFFE07D1/00010000/E0000211D831B603");
	//test_testStat(fatx, "/");
	//test_splitPath("/a");
	//test_findFreeCluster(fatx, 0);
	//test_findFirstFreeDirEntry(fatx, "");
	test_write(fatx, "/abc");
	fatx_free(fatx);
	return 0;
}
