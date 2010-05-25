/**
 * \file libfatxutils.c
 * \author Tim Wu
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/disk.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "libfatx_internal.h"

uint32_t
fatx_calcClusters(int dev)
{
	struct stat statBuf;
	uint32_t totalClusters = 0;
	off_t blockCount = 0;
	size_t blockSize = 0;
	if (fstat(dev, &statBuf)) {
		printf("failed to stat. errno %d\n", errno);
		return 0;
	}
	blockSize = statBuf.st_blksize;
	if(S_ISREG(statBuf.st_mode)) {
		totalClusters = statBuf.st_size >> 14;
	} else if(S_ISBLK(statBuf.st_mode)) {
#if (__APPLE__)
		ioctl(dev, DKIOCGETBLOCKCOUNT, &blockCount);
		ioctl(dev, DKIOCGETBLOCKSIZE, &blockSize);
#else 
		ioctl(dev, BLKGETSIZE, &blockCount);
		ioctl(dev, BLKBSZGET, &blockSize);
#endif
		totalClusters = blockCount / (FAT_CLUSTER_SZ / blockSize);
	} else {
		totalClusters = 0;
	}
	return totalClusters;
}
 
off_t
fatx_calcDataStart(int fatType, uint32_t clusters)
{
	size_t fatSize = clusters << fatType;
	fatSize += fatSize & (FAT_PAGE_SZ - 1) ? FAT_PAGE_SZ : 0;
	fatSize = fatSize & (~(FAT_PAGE_SZ - 1));
	// For some reason the first page after the FAT counts as page 1.
	// So we'll just be skipping it
	return FAT_OFFSET + fatSize - FAT_CLUSTER_SZ;	
}

uint32_t
fatx_calcFatPages(off_t dataStart)
{
	return (dataStart - FAT_OFFSET) / FAT_PAGE_SZ;
}

uint32_t
fatx_readFatEntry(fatx_handle * fatx_h, 
					   uint32_t      clusterNo)
{
	uint32_t pageNo, entryNo, entry;
	fatx_fat_cache_entry * cacheEntry;
	FATX_LOCK(fatx_h);
	if (fatx_h->fatType == FATX32) {
		pageNo = clusterNo / FATX32_ENTRIES_PER_PAGE;
		entryNo = clusterNo & (FATX32_ENTRIES_PER_PAGE - 1);
		cacheEntry = fatx_getFatPage(fatx_h, pageNo);
		entry = SWAP32(cacheEntry->fatx32Entries[entryNo]);
	} else {
		pageNo = clusterNo / FATX16_ENTRIES_PER_PAGE;
		entryNo = clusterNo & (FATX16_ENTRIES_PER_PAGE - 1);
		cacheEntry = fatx_getFatPage(fatx_h, pageNo);
		entry = SWAP16(cacheEntry->fatx16Entries[entryNo]);
	}
	FATX_UNLOCK(fatx_h);
	return entry;
}

void
fatx_writeFatEntry(fatx_handle *fatx_h,
						 uint32_t     clusterNo,
						 uint32_t     value)
{
	uint32_t pageNo, entryNo;
	fatx_fat_cache_entry * cacheEntry;
	FATX_LOCK(fatx_h);
	if (fatx_h->fatType == FATX32) {
		pageNo = clusterNo / FATX32_ENTRIES_PER_PAGE;
		entryNo = clusterNo & (FATX32_ENTRIES_PER_PAGE - 1);
		cacheEntry = fatx_getFatPage(fatx_h, pageNo);
		cacheEntry->dirty = 1;
		cacheEntry->fatx32Entries[entryNo] = SWAP32(value);
	} else {
		pageNo = clusterNo / FATX16_ENTRIES_PER_PAGE;
		entryNo = clusterNo & (FATX16_ENTRIES_PER_PAGE - 1);
		cacheEntry = fatx_getFatPage(fatx_h, pageNo);
		cacheEntry->dirty = 1;
		cacheEntry->fatx16Entries[entryNo] = SWAP16(value);
	}
	FATX_UNLOCK(fatx_h);
}

uint32_t
fatx_findFreeCluster(fatx_handle * fatx_h,
                     uint32_t      startClusterNo)
{
	startClusterNo = (startClusterNo + 1) % fatx_h->nClusters;
	FATX_LOCK(fatx_h);
	while(!IS_FREE_CLUSTER(fatx_readFatEntry(fatx_h, startClusterNo)))
		startClusterNo = (startClusterNo + 1) % fatx_h->nClusters;
	FATX_UNLOCK(fatx_h);
	return startClusterNo;
}

char
fatx_isEOC(fatx_handle * fatx_h,
			  uint32_t      clusterNo)
{
	if(fatx_h->fatType == FATX32) 
		return (clusterNo >= 0xFFFFFFF8);
	else 
		return (clusterNo >= 0xFFF8);
}

fatx_fat_cache_entry *
fatx_getFatPage(fatx_handle * fatx_h,
					 uint32_t      pageNo)
{
	fatx_fat_cache_entry * entry = NULL;
	FATX_LOCK(fatx_h);
	entry = fatx_h->fatCache + (pageNo % FAT_CACHE_SIZE);
	if(entry->pageNo != pageNo) {
		if(entry->dirty) fatx_flushFatCacheEntry(fatx_h, entry);
		fatx_loadFatPage(fatx_h, pageNo);
	}
	FATX_UNLOCK(fatx_h);
	return entry;
}

void
fatx_loadFatPage(fatx_handle * fatx_h,
					  uint32_t      pageNo)
{
	fatx_fat_cache_entry * entry = fatx_h->fatCache + (pageNo % FAT_CACHE_SIZE);
	int                    i;
	uint16_t               lastFreeCluster;
	FATX_LOCK(fatx_h);
	lseek(fatx_h->dev, FAT_OFFSET + (pageNo * FAT_PAGE_SZ), SEEK_SET);
	read(fatx_h->dev, entry->data, FAT_PAGE_SZ);
	entry->dirty = 0;
	entry->pageNo = pageNo;
	FATX_UNLOCK(fatx_h);
}

void
fatx_flushFatCacheEntry(fatx_handle          * fatx_h,
								fatx_fat_cache_entry * cacheEntry)
{
	FATX_LOCK(fatx_h);
	lseek(fatx_h->dev, FAT_OFFSET + (cacheEntry->pageNo * FAT_PAGE_SZ), SEEK_SET);
	write(fatx_h->dev, cacheEntry->data, FAT_PAGE_SZ);
	cacheEntry->dirty = 0;
	FATX_UNLOCK(fatx_h);
}

fatx_cache_entry * 
fatx_getCluster(fatx_handle * fatx_h, 
				    uint32_t      clusterNo)
{
	fatx_cache_entry * cacheEntry;
	FATX_LOCK(fatx_h);
	cacheEntry = &fatx_h->cache[clusterNo % CACHE_SIZE];
	if(cacheEntry->clusterNo != clusterNo) {
		if(cacheEntry->dirty) fatx_flushClusterCacheEntry(fatx_h, cacheEntry);
		fatx_loadCluster(fatx_h, clusterNo);
	}
	FATX_UNLOCK(fatx_h);
	return cacheEntry;
}

void
fatx_flushClusterCacheEntry(fatx_handle      * fatx_h,
									 fatx_cache_entry * cacheEntry)
{
	FATX_LOCK(fatx_h);
	off_t fileOffset = cacheEntry->clusterNo;
	fileOffset *= FAT_CLUSTER_SZ;
	lseek(fatx_h->dev, fatx_h->dataStart + fileOffset, SEEK_SET);
	write(fatx_h->dev, cacheEntry->data, FAT_CLUSTER_SZ);
	cacheEntry->dirty = 0;
	FATX_UNLOCK(fatx_h);
}

void 
fatx_loadCluster(fatx_handle * fatx_h, 
					  uint32_t      clusterNo)
{
	FATX_LOCK(fatx_h);
	fatx_cache_entry * cacheEntry = &fatx_h->cache[clusterNo % CACHE_SIZE];
	off_t fileOffset = clusterNo;
	fileOffset *= FAT_CLUSTER_SZ;
	lseek(fatx_h->dev, fatx_h->dataStart + fileOffset, SEEK_SET);
	read(fatx_h->dev, cacheEntry->data, FAT_CLUSTER_SZ);
	cacheEntry->clusterNo = clusterNo;
	cacheEntry->dirty = 0;
	FATX_UNLOCK(fatx_h);
}

void
fatx_freeFilenameList(fatx_filename_list * fnList)
{
	fatx_filename_list * next;
	for(;fnList != NULL; fnList = next) {
		next = fnList->next;
		free(fnList);
	}
}

fatx_filename_list * 
fatx_splitPath(const char * path)
{
	fatx_filename_list * list = NULL;
	char *               filename;
	if (path == NULL || !strncmp(path, "", 1) || !strncmp(path, "/", 2)) {
		return NULL;
	} else {
		list = (fatx_filename_list *) malloc(sizeof(fatx_filename_list));
		filename = list->filename;
		// Skip leading /'s
		while(*path == '/') path++;
		while(*path != '/' && *path != '\0') {
			*filename = *path;
			filename++;
			path++;
		}
		*filename = '\0';
		list->next = fatx_splitPath(path);
	}
	return list;
}

fatx_filename_list *
fatx_basename(fatx_filename_list * path)
{
	fatx_filename_list * basename;
	if(path == NULL) return NULL;
	while(path->next != NULL) {
		path = path->next;
	}
	basename = (fatx_filename_list *) malloc(sizeof(fatx_filename_list));
	memcpy(basename, path, sizeof(fatx_filename_list));
	return basename;
}

fatx_filename_list *
fatx_dirname(fatx_filename_list * path)
{
	fatx_filename_list * dirname;
	if(path == NULL || path->next == NULL) return NULL;
	dirname = (fatx_filename_list *) malloc(sizeof(fatx_filename_list));
	memcpy(dirname, path, sizeof(fatx_filename_list));
	dirname->next = fatx_dirname(path->next);
	return dirname;
}

void
fatx_printFilenameList(fatx_filename_list * fnList)
{
	if(fnList == NULL) {
		printf("/\n");
		return;
	}
	while(fnList != NULL) {
		printf("/%s", fnList->filename);
		fnList = fnList->next;
	}
	printf("\n");
}

fatx_dir_iter *
fatx_createDirIter(fatx_handle *          fatx_h,
						 fatx_directory_entry * directoryEntry)
{
	fatx_dir_iter        * iter = NULL;
	uint32_t               clusterNo = 1;
	FATX_LOCK(fatx_h);
	if (directoryEntry != NULL) {
		if(!IS_VALID_ENTRY(directoryEntry) || !IS_FOLDER(directoryEntry))
			goto finish;
		clusterNo = SWAP32(directoryEntry->firstCluster);
	} 
	iter = (fatx_dir_iter *) malloc(sizeof(fatx_dir_iter));
	iter->fatx_h = fatx_h;
	iter->entryNo = 0;
	iter->clusterNo = clusterNo;
	iter->dirEntList = NULL;
finish:
	FATX_UNLOCK(fatx_h);
	return iter;
}

fatx_directory_entry *
fatx_readDirectoryEntry(fatx_handle *   fatx_h,
								fatx_dir_iter * iter)
{
	fatx_directory_entry   * directoryEntry = NULL;
	fatx_cache_entry       * cacheEntry;
	uint32_t                 nextCluster;
	FATX_LOCK(iter->fatx_h);
	if(iter->entryNo == DIR_ENTRIES_PER_CLUSTER) {
		nextCluster = fatx_readFatEntry(fatx_h, iter->clusterNo);
		if (fatx_isEOC(fatx_h, nextCluster)) goto finish;
		iter->entryNo = 0;
		iter->clusterNo = nextCluster;
	}
	cacheEntry = fatx_getCluster(iter->fatx_h, iter->clusterNo);
	directoryEntry = &cacheEntry->dirEntries[iter->entryNo];
	if(directoryEntry->filenameSz == 0xFF) {
		directoryEntry = NULL;
		goto finish;
	}
	iter->entryNo++;
finish:
	FATX_UNLOCK(iter->fatx_h);
	return directoryEntry;
}

fatx_directory_entry *
fatx_findDirectoryEntry(fatx_handle *          fatx_h,
								fatx_filename_list *   fnList,
								fatx_directory_entry * baseDirectoryEntry)
{
	fatx_dir_iter *        iter;
	fatx_directory_entry * directoryEntry = NULL;
	FATX_LOCK(fatx_h);
	if (fnList == NULL) {
		directoryEntry = baseDirectoryEntry;
		goto finish;
	}
	iter = fatx_createDirIter(fatx_h, baseDirectoryEntry);
	while( (directoryEntry = fatx_readDirectoryEntry(fatx_h, iter)) ) {
		if(!IS_VALID_ENTRY(directoryEntry)) continue;
		if(!strncmp(directoryEntry->filename, fnList->filename, directoryEntry->filenameSz)) {
			directoryEntry = fatx_findDirectoryEntry(fatx_h, fnList->next, directoryEntry);
			break;
		}
	}
	fatx_closedir(iter);
finish:
	FATX_UNLOCK(fatx_h);
	return directoryEntry;
}

time_t
fatx_makeTimeType(uint16_t date, uint16_t time)
{
	struct tm time_struct =  {
					                .tm_year = ((date & 0xFE00) >> 9) + 80,
										 .tm_mon = ((date & 0x01E0) >> 5) - 1,
										 .tm_mday = date & 0x1F,
										 .tm_hour = time >> 11,
										 .tm_min = (time >> 5) & 0x3F,
										 .tm_sec = (time & 0x1F) << 1,
										 .tm_isdst = -1,
										 .tm_wday = 0,
										 .tm_yday = 0,																								         };
	return mktime(&time_struct);
}

int
fatx_readFromDirectoryEntry(fatx_handle          * fatx_h,
									 fatx_directory_entry * directoryEntry,
									 char                 * buf,
									 off_t                  offset,
									 size_t                 len)
{
	uint32_t                    currentClusterNo = SWAP32(directoryEntry->firstCluster);
	uint32_t                    fileClusterNo    = (offset / FAT_CLUSTER_SZ);
	uint32_t                    i, bytesRead = 0, retVal;
	fatx_cache_entry          * cacheEntry       = NULL;
	if(offset >= SWAP32(directoryEntry->fileSize)) {
		return -EOVERFLOW;
	}
	len = MIN(len, SWAP32(directoryEntry->fileSize) - offset);
	retVal = len;
	offset = offset % FAT_CLUSTER_SZ;
	FATX_LOCK(fatx_h);
	for(i = 0; i < fileClusterNo; i++) {
		currentClusterNo = fatx_readFatEntry(fatx_h, currentClusterNo);
		if(fatx_isEOC(fatx_h, currentClusterNo) || IS_FREE_CLUSTER(currentClusterNo)) {
			retVal = -EBADF;
			goto finish;
		}
	}
	while(len > 0) {
		if(fatx_isEOC(fatx_h, currentClusterNo) || IS_FREE_CLUSTER(currentClusterNo)) {
			retVal = -EBADF;
			goto finish;
		}
		bytesRead = MIN(len, FAT_CLUSTER_SZ - offset);
		cacheEntry = fatx_getCluster(fatx_h, currentClusterNo);
		memcpy(buf, cacheEntry->data + offset, bytesRead);
		len -= bytesRead;
		buf += bytesRead;
		offset = 0;
		currentClusterNo = fatx_readFatEntry(fatx_h, currentClusterNo);
	}
finish:
	FATX_UNLOCK(fatx_h);
	return retVal;
}

int
fatx_writeToDirectoryEntry(fatx_handle          * fatx_h,
									fatx_directory_entry * directoryEntry,
									char                 * buf,
									off_t                  offset,
									size_t                 len)
{
	uint32_t                    currentClusterNo = SWAP32(directoryEntry->firstCluster);
	uint32_t                    fileClusterNo    = (offset / FAT_CLUSTER_SZ);
	uint32_t                    i, bytesWrite = 0, retVal;
	uint32_t                    nextClusterNo = 0;
	uint32_t                    filesize = offset;
	fatx_cache_entry          * cacheEntry       = NULL;
	if(offset > SWAP32(directoryEntry->fileSize)) {
		return -EOVERFLOW;
	}
	retVal = len;
	offset = offset % FAT_CLUSTER_SZ;
	FATX_LOCK(fatx_h);
	for(i = 0; i < fileClusterNo; i++) {
		currentClusterNo = fatx_readFatEntry(fatx_h, currentClusterNo);
		if(fatx_isEOC(fatx_h, currentClusterNo) || IS_FREE_CLUSTER(currentClusterNo)) {
			retVal = -EBADF;
			goto finish;
		}
	}
	while(len > 0) {
		bytesWrite = MIN(len, FAT_CLUSTER_SZ - offset);
		cacheEntry = fatx_getCluster(fatx_h, currentClusterNo);
		memcpy(cacheEntry->data + offset, buf, bytesWrite);
		cacheEntry->dirty = 1;
		len -= bytesWrite;
		buf += bytesWrite;
		filesize += bytesWrite;
		offset = 0;
		nextClusterNo = fatx_readFatEntry(fatx_h, currentClusterNo);
		if(fatx_isEOC(fatx_h, nextClusterNo)) {
			nextClusterNo = fatx_findFreeCluster(fatx_h, currentClusterNo);
			if (nextClusterNo == 0) {
				retVal = -ENOSPC;
				goto finish;
			}
			fatx_writeFatEntry(fatx_h, currentClusterNo, nextClusterNo);
		}
		currentClusterNo = nextClusterNo;
	}
finish:
	directoryEntry->fileSize = SWAP32(MAX(filesize, SWAP32(directoryEntry->fileSize)));
	FATX_UNLOCK(fatx_h);
	return retVal;
}

void
fatx_initDirCluster(fatx_handle * fatx_h,
                    uint32_t      clusterNo)
{
	fatx_cache_entry * cacheEntry;
	uint32_t           i;
	FATX_LOCK(fatx_h);
	cacheEntry = fatx_getCluster(fatx_h, clusterNo);
	cacheEntry->dirty = 1;
	for(i = 0; i < DIR_ENTRIES_PER_CLUSTER; i++) {
		cacheEntry->dirEntries[i].filenameSz = 0xFF;
	}
	FATX_UNLOCK(fatx_h);
}

fatx_directory_entry *
fatx_getFirstOpenDirectoryEntry(fatx_handle * fatx_h, fatx_directory_entry * folder)
{
	fatx_directory_entry * entry = NULL;
	uint32_t               freeCluster;
	fatx_cache_entry     * cacheEntry;
	FATX_LOCK(fatx_h);
	fatx_dir_iter * iter = fatx_createDirIter(fatx_h, folder);
	while ( (entry = fatx_readDirectoryEntry(fatx_h, iter)) ) {
		if(!IS_VALID_ENTRY(entry)) goto finish;
	}
	if(entry == NULL) {
		if(iter->entryNo == DIR_ENTRIES_PER_CLUSTER &&
			// Hit the last spot the last cluster of a folder. need to make a new one.
		   fatx_isEOC(fatx_h, fatx_readFatEntry(fatx_h, iter->clusterNo))) {
			freeCluster = fatx_findFreeCluster(fatx_h, iter->clusterNo);
			if(freeCluster == 0) goto finish;
			fatx_writeFatEntry(fatx_h, iter->clusterNo, SWAP32(freeCluster));
			fatx_initDirCluster(fatx_h, freeCluster);
			cacheEntry = fatx_getCluster(fatx_h, freeCluster);
			entry = cacheEntry->dirEntries;
		} else {
			// somewhere at the end of a folder.
			cacheEntry = fatx_getCluster(fatx_h, iter->clusterNo);
			entry = cacheEntry->dirEntries + iter->entryNo;
			cacheEntry->dirty = 1;
		}
	}
finish:
	FATX_UNLOCK(fatx_h);
	return entry;
}

