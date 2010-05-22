#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "libfatx.h"
#include "libfatx_internal.h"

fatx_t
fatx_init(const char * path)
{  
   fatx_handle * fatx = (fatx_handle *) malloc(sizeof(fatx_handle));
   if((fatx->dev = open(path, O_RDONLY)) <= 0)
		goto error;
   if(pthread_mutex_init(&fatx->devLock, NULL)) 
		goto error;
   fatx->nClusters = fatx_calcClusters(fatx->dev);
	fatx->fatType = fatx->nClusters < FATX32_MIN_CLUSTERS ? FATX16 : FATX32;
	fatx->dataStart = fatx_calcDataStart(fatx->fatType, fatx->nClusters);
   return (fatx_t) fatx;

error:
	pthread_mutex_destroy(&fatx->devLock);
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
	close(fatx->dev);
	free(fatx);
}

void
fatx_printInfo(fatx_t fatx)
{
	fatx_handle * fatx_h = (fatx_handle *) fatx;
	printf("Fatx handle:\n");
	printf("\tdev fd = %d\n", fatx_h->dev);
	printf("\tnClusters = %lu\n", fatx_h->nClusters);
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
