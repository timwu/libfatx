/**
 * \file libfatx_internal.h
 * \author Tim wu
 */

#include <pthread.h>

#ifndef __LIBFATX_INTERNAL_H__
#define __LIBFATX_INTERNAL_H__

/** Types of FATX's */
enum FAT_TYPE {
	/** FATX 16, entries are 16 bits */
   FATX16 = 1,
	/** FATX32, entries are 32 bits */
   FATX32  
};

/** Minimum number of clusters for a partition to be FATX32 (~1GB in size) */
#define FATX32_MIN_CLUSTERS 65525L

/** Start of the FAT table */
#define FAT_OFFSET 0x1000L

/** Size of a FAT cluster */
#define FAT_CLUSTER_SZ 0x4000L

typedef struct fatx_handle {
   /** File descriptor of the device */
   int             dev; 
   /** Lock to synchronize access to the device */
   pthread_mutex_t devLock; 
   /** Number of clusters in the partition */
   uint64_t        nClusters; 
   /** FAT type, either fat16 or fat32 */
   enum FAT_TYPE   fatType; 
	/** Offset to the start of the data. */
	off_t				 dataStart;
} fatx_handle;

/**
 * Calculate the number of clusters in a fatx device.
 *
 * \param dev fd of the device to check.
 * \return number of clusters in the fatx device.
 */
uint64_t fatx_calcClusters(int dev);

/**
 * Calculate the start of the data clusters.
 *
 * \param fatType the type of fat, 1 for FATX16, 2 for FATX32
 * \param clusters number of clusters.
 * \return the starting offset of the first data cluster
 */
off_t fatx_calcDataStart(int fatType, uint64_t clusters);

/**
 * Retrieve a FAT entry
 *
 * \param fatx_h the fatx object.
 * \param entryNo the entry to retrieve.
 * \return fatx entry
 */
uint32_t fatx_readFatEntry(fatx_handle * fatx_h, off_t entryNo);

#endif // __LIBFATX_INTERNAL_H__
