#include <pthread.h>
#include <stdint.h>
#include "libfatx.h"

enum FAT_TYPE {
  FATX16,
  FATX32 
};

struct fatx_handle {
  int             dev; /**< File descriptor of the device */
  pthread_mutex_t devLock; /**< Lock to synchronize access to the device */
  uint64_t        nClusters; /**< Number of clusters in the partition */
  enum FAT_TYPE   fatType; /**< FAT type, either fat16 or fat32 */
  void *          fat; /**< Pointer to the fat table in memory. */
};

fatx_t
fatx_init(const char * path)
{  
  return NULL;
}

void
fatx_free(fatx_t fatx)
{
  
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
