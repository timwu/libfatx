/**
 * \file libfatxutils.c
 * \author Tim Wu
 */
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/disk.h>
#include <errno.h>
#include "libfatx_internal.h"


uint64_t
fatx_calcClusters(int dev)
{
	struct stat statBuf;
	uint64_t totalClusters = 0;
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
fatx_calcDataStart(int fatType, uint64_t clusters)
{
	size_t fatSize = clusters << fatType;
	fatSize += fatSize & 0xFFFL ? 4096 : 0;
	fatSize = fatSize & (~0xFFFL);
	return FAT_OFFSET + fatSize;	
}
