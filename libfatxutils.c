/**
 * \file libfatxutils.c
 * \author Tim Wu
 */
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/disk.h>
#include <errno.h>

uint64_t
fatx_calcClusters(int dev)
{
	struct stat statBuf;
	uint64_t totalClusters = 0;
	off_t blockCount = 0;
	if (fstat(dev, &statBuf)) {
		printf("failed to stat. errno %d\n", errno);
		return 0;
	}
	if(S_ISREG(statBuf.st_mode)) {
		totalClusters = statBuf.st_size >> 14;
	} else if(S_ISBLK(statBuf.st_mode)) {
#if (__APPLE__)
		ioctl(dev, DKIOCGETBLOCKCOUNT, &blockCount);
#else 
		ioctl(dev, BLKGETSIZE, &blockCount);
#endif
		totalClusters = blockCount >> 3;
	} else {
		totalClusters = 0;
	}
	return totalClusters;
}
 
off_t
fatx_calcFatSize(int entrySize, uint64_t clusters)
{
	uint64_t fatSize = clusters << entrySize;
	fatSize += fatSize & 0xFFFL ? 4096 : 0;
	fatSize = fatSize & (~0xFFFL);
	return fatSize;	
}
