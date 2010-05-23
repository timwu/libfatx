#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "libfatx_internal.h"
#include "libfatx.h"

fatx_t
fatx_init(const char * path)
{  
   fatx_handle * fatx = (fatx_handle *) calloc(1, sizeof(fatx_handle));
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
	fatx_dir_iter *      dirIter = NULL;
	fatx_dir_iter *      rootDirIter = NULL;
	fatx_filename_list * fnList = NULL;
	FATX_LOCK(fatx);
	rootDirIter = (fatx_dir_iter *) calloc(1, sizeof(fatx_dir_iter));
	rootDirIter->fatx_h = fatx;
	fnList = fatx_splitPath(path);
	dirIter = fatx_createDirIter(fatx, fnList, rootDirIter);
	fatx_freeFilenameList(fnList);
	FATX_UNLOCK(fatx);
	return (fatx_dir_iter_t) dirIter;
}

fatx_dirent_t * 
fatx_readdir(fatx_dir_iter_t iter)
{
	fatx_cache_entry * 	  cacheEntry;
	fatx_dirent_t *        dirent = NULL;
	fatx_directory_entry * entryList = NULL;
	fatx_dirent_list *     direntList = NULL;
	uint8_t 					  filenameSize;
	FATX_LOCK(iter->fatx_h);
	if(iter->entryNo == DIR_ENTRIES_PER_CLUSTER) {
		iter->entryNo = 0;
		iter->clusterNo = fatx_readFatEntry(iter->fatx_h, iter->clusterNo);
	}
	if(fatx_isEOC(iter->fatx_h, iter->clusterNo)) goto finish;
	cacheEntry = fatx_getCluster(iter->fatx_h, iter->clusterNo);
	entryList = (fatx_directory_entry *) cacheEntry->data;
	if(entryList[iter->entryNo].filenameSz == 0xFF) goto finish;
	else if(entryList[iter->entryNo].filenameSz > 42) {
		iter->entryNo++;
		dirent = fatx_readdir(iter);
	} else {
		filenameSize = entryList[iter->entryNo].filenameSz;
		dirent = (fatx_dirent_t *) malloc(sizeof(fatx_dirent_t));
		dirent->d_namelen = filenameSize;
		dirent->d_name = (char *) malloc(filenameSize + 1);
		strncpy(dirent->d_name, entryList[iter->entryNo].filename, filenameSize);
		dirent->d_name[filenameSize] = '\0';
		direntList = (fatx_dirent_list *) malloc(sizeof(fatx_dirent_list));
		direntList->dirEnt = dirent;
		direntList->next = iter->dirEntList;
		iter->dirEntList = direntList;
		iter->entryNo++;
	}
finish:
	FATX_UNLOCK(iter->fatx_h);
	return dirent;
}

void 
fatx_closedir(fatx_dir_iter_t iter)
{
	fatx_dir_iter * dirIter = (fatx_dir_iter *) iter;
	fatx_dirent_list* nextDirEntList;
	for(;iter->dirEntList != NULL; iter->dirEntList = nextDirEntList) {
		nextDirEntList = iter->dirEntList->next;
		free(iter->dirEntList->dirEnt->d_name);
		free(iter->dirEntList->dirEnt);
		free(iter->dirEntList);
	}
	free(dirIter);
}

