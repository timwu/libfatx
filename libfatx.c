#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "libfatx.h"
#include "libfatx_internal.h"

fatx_t
fatx_init(const char * path)
{  
   fatx_handle * fatx = (fatx_handle *) calloc(1, sizeof(fatx_handle));
	int i = 0;
   if((fatx->dev = open(path, O_RDONLY)) <= 0)
		goto error;
	if(pthread_mutexattr_init(&fatx->mutexAttr))
		goto error;
	if(pthread_mutexattr_settype(&fatx->mutexAttr, PTHREAD_MUTEX_RECURSIVE))
		goto error;
   if(pthread_mutex_init(&fatx->devLock, &fatx->mutexAttr)) 
		goto error;
   fatx->nClusters = fatx_calcClusters(fatx->dev);
	fatx->fatType = fatx->nClusters < FATX32_MIN_CLUSTERS ? FATX16 : FATX32;
	fatx->dataStart = fatx_calcDataStart(fatx->fatType, fatx->nClusters);
	// Load up the 0'th pages to intialize the caches
	fatx_loadCluster(fatx, 0);
	fatx_loadFatPage(fatx, 0);
   return (fatx_t) fatx;

error:
	pthread_mutex_destroy(&fatx->devLock);
	pthread_mutexattr_destroy(&fatx->mutexAttr);
	if (fatx->dev) close(fatx->dev);
	free(fatx);
	return NULL;
}

void
fatx_free(fatx_t fatx)
{
	int i;
	if (fatx == NULL)
		return;
	pthread_mutex_destroy(&fatx->devLock);
	pthread_mutexattr_destroy(&fatx->mutexAttr);
	close(fatx->dev);
	free(fatx);
}

void
fatx_printInfo(fatx_t fatx)
{
	fatx_handle * fatx_h = (fatx_handle *) fatx;
	printf("Fatx handle:\n");
	printf("\tdev fd = %d\n", fatx_h->dev);
	printf("\tnClusters = %u\n", fatx_h->nClusters);
	printf("\tfatType = %d\n", fatx_h->fatType);
	printf("\tdataStart = 0x%lx\n", fatx_h->dataStart);
}

int
fatx_read(fatx_t      fatx, 
			 const char* path, 
	  		 char*       buf, 
	       off_t       offset, 
	       size_t      size)
{
   return 0;
}

int
fatx_write(fatx_t      fatx,
	        const char* path,
	        const char* buf,
	        off_t       offset,
	        size_t      size) 
{
   return 0;
}

int
fatx_stat(fatx_t       fatx, 
	       const char*  path, 
	       struct stat* st_buf)
{
   return 0;
}

int
fatx_remove(fatx_t      fatx, 
	         const char* path)
{
   return 0;
}

int
fatx_mkfile(fatx_t      fatx, 
	         const char* path)
{
   return 0;
}

int
fatx_mkdir(fatx_t      fatx, 
	        const char* path)
{
   return 0;
}

fatx_dir_iter_t 
fatx_opendir(fatx_t      fatx, 
				 const char* path)
{
	fatx_dir_iter * dirIter = NULL;
	return (fatx_dir_iter_t) dirIter;
}

fatx_dirent_t 
fatx_readdir(fatx_dir_iter_t iter)
{
	return NULL;
}

void 
fatx_closedir(fatx_dir_iter_t iter)
{
	fatx_dir_iter * dirIter = (fatx_dir_iter *) iter;
	if(dirIter->path) free(dirIter->path);
	fatx_freeFilenameList(dirIter->fnList);
	free(dirIter);
}

