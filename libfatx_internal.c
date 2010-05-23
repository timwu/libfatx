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
fatx_readFatEntry(fatx_handle * fatx_h, 
					   uint32_t      clusterNo)
{
	uint32_t pageNo, entryNo, entry;
	fatx_fat_cache_entry * cacheEntry;
	FATX_LOCK(fatx_h);
	if (fatx_h->fatType == FATX32) {
		pageNo = clusterNo / FATX32_ENTRIES_PER_PAGE;
		entryNo = clusterNo & (FATX32_ENTRIES_PER_PAGE - 1);
		cacheEntry = &fatx_h->fatCache[pageNo % FAT_CACHE_SIZE];
		if(cacheEntry->pageNo != pageNo) {
			if(cacheEntry->dirty) fatx_flushFatPage(fatx_h, pageNo);
			fatx_loadFatPage(fatx_h, pageNo);
		}
		entry = SWAP32(cacheEntry->fatx32Entries[entryNo]);
	} else {
		pageNo = clusterNo / FATX16_ENTRIES_PER_PAGE;
		entryNo = clusterNo & (FATX16_ENTRIES_PER_PAGE - 1);
		cacheEntry = &fatx_h->fatCache[pageNo % FAT_CACHE_SIZE];
		if(cacheEntry->pageNo != pageNo) {
			if(cacheEntry->dirty) fatx_flushFatPage(fatx_h, pageNo);
			fatx_loadFatPage(fatx_h, pageNo);
		}
		entry = SWAP16(cacheEntry->fatx16Entries[entryNo]);
	}
	FATX_UNLOCK(fatx_h);
	return entry;
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

void
fatx_loadFatPage(fatx_handle * fatx_h,
					  uint32_t      pageNo)
{
	FATX_LOCK(fatx_h);
	lseek(fatx_h->dev, FAT_OFFSET + (pageNo * FAT_PAGE_SZ), SEEK_SET);
	read(fatx_h->dev, fatx_h->fatCache[pageNo % FAT_CACHE_SIZE].data, FAT_PAGE_SZ);
	fatx_h->fatCache[pageNo % FAT_CACHE_SIZE].dirty = 0;
	fatx_h->fatCache[pageNo % FAT_CACHE_SIZE].pageNo = pageNo;
	FATX_UNLOCK(fatx_h);
}

void
fatx_flushFatPage(fatx_handle * fatx_h,
						uint32_t      pageNo)
{

}

fatx_cache_entry * 
fatx_getCluster(fatx_handle * fatx_h, 
				    uint32_t      clusterNo)
{
	fatx_cache_entry * cacheEntry;
	FATX_LOCK(fatx_h);
	cacheEntry = &fatx_h->cache[clusterNo % CACHE_SIZE];
	if(cacheEntry->clusterNo != clusterNo) {
		if(cacheEntry->dirty) fatx_flushCluster(fatx_h, clusterNo);
		fatx_loadCluster(fatx_h, clusterNo);
	}
	FATX_UNLOCK(fatx_h);
	return cacheEntry;
}

void
fatx_flushCluster(fatx_handle * fatx_h,
						uint32_t      clusterNo)
{

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
	FATX_LOCK(iter->fatx_h);
	if(iter->entryNo == DIR_ENTRIES_PER_CLUSTER) {
		iter->entryNo = 0;
		iter->clusterNo = fatx_readFatEntry(iter->fatx_h, iter->clusterNo);
	}
	if(fatx_isEOC(iter->fatx_h, iter->clusterNo)) goto finish;
	cacheEntry = fatx_getCluster(iter->fatx_h, iter->clusterNo);
	directoryEntry = &((fatx_directory_entry *) cacheEntry->data)[iter->entryNo];
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
										 .tm_yday = 0,																																	          };
	return mktime(&time_struct);
}
