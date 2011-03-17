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
fatx_init(const char     * path,
          fatx_options_t * options)
{  
   fatx_handle * fatx = (fatx_handle *) calloc(1, sizeof(fatx_handle));
   if(options == NULL)
      goto error;
   memcpy(&fatx->options, options, sizeof(fatx_options_t));
   fatx->options.filePerm &= 0777; // only the file permissions are allowed here.
   if((fatx->dev = open(path, O_RDWR)) <= 0)
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
   fatx->noFatPages = fatx_calcFatPages(fatx->dataStart);
   // Load up the 0'th pages to intialize the caches
   fatx_loadFatPage(fatx, 0);
   fatx->rootDirEntry.firstCluster = SWAP32(1);
   fatx->rootDirEntry.attributes = 0x10;
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
   int i = 0;
   if (fatx == NULL)
      return;
   for(i = 0; i < CACHE_SIZE; i++) {
      if(fatx->cache[i].dirty)
         fatx_flushClusterCacheEntry(fatx, fatx->cache + i);
   }
   for(i = 0; i < FAT_CACHE_SIZE; i++) {
      if(fatx->fatCache[i].dirty)
         fatx_flushFatCacheEntry(fatx, fatx->fatCache + i);
   }
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
   printf("\tnoFatPages = 0x%x\n", fatx_h->noFatPages);
}

int
fatx_read(fatx_t      fatx, 
          const char* path, 
          char*       buf, 
          off_t       offset, 
          size_t      size)
{
   fatx_directory_entry * directoryEntry;
   fatx_filename_list   * fnList = NULL;
   int                    retVal = 0;
   fnList = fatx_splitPath(path);
   if(fnList == NULL) {
      retVal = -ENOENT;
      goto finish;
   }
   FATX_LOCK(fatx);
   directoryEntry = fatx_findDirectoryEntry(fatx, fnList, &fatx->rootDirEntry);
   if(directoryEntry == NULL) {
      retVal = -ENOENT;
      goto finish;
   }
   retVal = fatx_readFromDirectoryEntry(fatx, directoryEntry, buf, offset, size);
finish:
   FATX_UNLOCK(fatx);
   fatx_freeFilenameList(fnList);
   return retVal;
}

int
fatx_write(fatx_t      fatx,
           const char* path,
           const char* buf,
           off_t       offset,
           size_t      size) 
{
   fatx_directory_entry * directoryEntry;
   fatx_filename_list   * fnList = NULL;
   int                    retVal = 0;
   fnList = fatx_splitPath(path);
   if(fnList == NULL) {
      // Writing into the root directory isn't possible.
      retVal = -ENOENT;
      goto finish;
   }
   FATX_LOCK(fatx);
   directoryEntry = fatx_findDirectoryEntry(fatx, fnList, &fatx->rootDirEntry);
   if(directoryEntry == NULL) {
      retVal = -ENOENT;
      goto finish;
   }
   retVal = fatx_writeToDirectoryEntry(fatx, directoryEntry, buf, offset, size);
finish:
   FATX_UNLOCK(fatx);
   fatx_freeFilenameList(fnList);
   return retVal;
}

int
fatx_stat(fatx_t       fatx, 
          const char*  path, 
          struct stat* st_buf)
{
   fatx_directory_entry * directoryEntry;
   fatx_filename_list *   fnList;
   int err = 0;
   FATX_LOCK(fatx);
   fnList = fatx_splitPath(path);
   if(fnList == NULL) {
      // Root directory.
      st_buf->st_mode = S_IFDIR | fatx->options.filePerm;
      st_buf->st_nlink = 1;
      st_buf->st_size = 0;
      st_buf->st_uid = fatx->options.user;
      st_buf->st_gid = fatx->options.group;
   } else {
      directoryEntry = fatx_findDirectoryEntry(fatx, fnList, &fatx->rootDirEntry);
      fatx_freeFilenameList(fnList);
      if(directoryEntry == NULL) {
         err = -ENOENT;
         goto finish;
      }
      st_buf->st_mode = IS_FOLDER(directoryEntry) ? S_IFDIR : S_IFREG;
      st_buf->st_mode |= fatx->options.filePerm;
      st_buf->st_nlink = 1;
      st_buf->st_size = SWAP32(directoryEntry->fileSize);
      st_buf->st_mtime = fatx_makeTimeType(SWAP16(directoryEntry->modificationDate), 
                                           SWAP16(directoryEntry->modificationTime));
      st_buf->st_atime = fatx_makeTimeType(SWAP16(directoryEntry->accessDate), 
                                           SWAP16(directoryEntry->accessTime));
      st_buf->st_uid = fatx->options.user;
      st_buf->st_gid = fatx->options.group;
   }
finish:
   FATX_UNLOCK(fatx);
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
   int                  err       = 0;
   uint32_t             newFileCluster = 0;
   fatx_directory_entry * folder  = &fatx->rootDirEntry;
   fatx_directory_entry * newFile = NULL;
   fatx_filename_list * splitPath = fatx_splitPath(path);
   fatx_filename_list * dirname   = fatx_dirname(splitPath);
   fatx_filename_list * basename  = fatx_basename(splitPath);
   FATX_LOCK(fatx);
   if(dirname != NULL) {
      // dirname isn't null so need to search for the folder
      folder = fatx_findDirectoryEntry(fatx, dirname, &fatx->rootDirEntry);
      if(folder == NULL) {
         err = -ENOENT;
         goto finish;
      }
   }
   if (fatx_findDirectoryEntry(fatx, basename, folder) != NULL) {
      // See if the file already exists.
      err = -ENOENT;
      goto finish;
   }
   newFile = fatx_getFirstOpenDirectoryEntry(fatx, folder);
   newFileCluster = fatx_findFreeCluster(fatx, SWAP32(folder->firstCluster));
   memset(newFile, 0, sizeof(fatx_directory_entry));
   newFile->filenameSz = strlen(basename->filename);
   memcpy(newFile->filename, basename->filename, 42);
   newFile->firstCluster = SWAP32(newFileCluster);
   fatx_writeFatEntry(fatx, newFileCluster, 
                      fatx->fatType == FATX32 ? SWAP32(0xFFFFFFF8) : SWAP16(0xFFF8));
finish:
   FATX_UNLOCK(fatx);
   fatx_freeFilenameList(splitPath);
   fatx_freeFilenameList(dirname);
   fatx_freeFilenameList(basename);
   return err;
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
   fatx_dir_iter *        dirIter = NULL;
   fatx_directory_entry * directoryEntry = NULL;
   fatx_filename_list *   fnList = NULL;
   FATX_LOCK(fatx);
   fnList = fatx_splitPath(path);
   if(fnList == NULL) {
      dirIter = fatx_createDirIter(fatx, NULL);
      goto finish;
   }
   directoryEntry = fatx_findDirectoryEntry(fatx, fnList, directoryEntry);
   if (directoryEntry == NULL) goto finish;
   dirIter = fatx_createDirIter(fatx, directoryEntry);
   fatx_freeFilenameList(fnList);
finish:
   FATX_UNLOCK(fatx);
   return (fatx_dir_iter_t) dirIter;
}

fatx_dirent_t * 
fatx_readdir(fatx_dir_iter_t iter)
{
   fatx_dirent_t *        dirent = NULL;
   fatx_directory_entry * directoryEntry = NULL;
   fatx_dirent_list *     direntList = NULL;
   if (iter == NULL) return NULL;
   FATX_LOCK(iter->fatx_h);
   directoryEntry = fatx_readDirectoryEntry(iter->fatx_h, iter);
   if(directoryEntry == NULL) goto finish;
   if(!IS_VALID_ENTRY(directoryEntry)) {
      dirent = fatx_readdir(iter);
      goto finish;
   }
   dirent = (fatx_dirent_t *) malloc(sizeof(fatx_dirent_t));
   dirent->d_namelen = directoryEntry->filenameSz;
   dirent->d_name = (char *) malloc(dirent->d_namelen + 1);
   strncpy(dirent->d_name, directoryEntry->filename, dirent->d_namelen);
   dirent->d_name[dirent->d_namelen] = '\0';
   direntList = (fatx_dirent_list *) malloc(sizeof(fatx_dirent_list));
   direntList->dirEnt = dirent;
   direntList->next = iter->dirEntList;
finish:
   FATX_UNLOCK(iter->fatx_h);
   return dirent;
}

void 
fatx_closedir(fatx_dir_iter_t iter)
{
   fatx_dir_iter * dirIter = (fatx_dir_iter *) iter;
   fatx_dirent_list* nextDirEntList;
   if(iter == NULL) return;
   for(;iter->dirEntList != NULL; iter->dirEntList = nextDirEntList) {
      nextDirEntList = iter->dirEntList->next;
      free(iter->dirEntList->dirEnt->d_name);
      free(iter->dirEntList->dirEnt);
      free(iter->dirEntList);
   }
   free(dirIter);
}

