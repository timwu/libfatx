/**
 * \file libfatx_internal.h
 * \author Tim wu
 */


#ifndef __LIBFATX_INTERNAL_H__
#define __LIBFATX_INTERNAL_H__

#include "libfatx.h"

#include <pthread.h>
#include <sys/types.h>
#include <stdint.h>

#if (__APPLE__)
#include <libkern/OSByteOrder.h>
#define SWAP32(x) OSSwapHostToBigInt32(x)
#define SWAP16(x) OSSwapHostToBigInt16(x)
#else //__APPLE__
// TODO: Do something for linux
#endif //__APPLE__

/** Types of FATX's */
enum FAT_TYPE {
	/** FATX 16, entries are 16 bits */
   FATX16 = 1,
	/** FATX32, entries are 32 bits */
   FATX32  
};

/** Number of directory entries in a cluster */
#define DIR_ENTRIES_PER_CLUSTER 64

/** Minimum number of clusters for a partition to be FATX32 (~1GB in size) */
#define FATX32_MIN_CLUSTERS 65525L

/** Start of the FAT table */
#define FAT_OFFSET 0x1000L

/** Size of a FAT cluster */
#define FAT_CLUSTER_SZ 0x4000L

/** Number of cache clusters */
#define CACHE_SIZE 0x20

/** Number of FAT cache pages */
#define FAT_CACHE_SIZE 0x20

/** Number of FATX32 entries in a page */
#define FATX32_ENTRIES_PER_PAGE 0x400

/** Number of FATX15 entries in a page */
#define FATX16_ENTRIES_PER_PAGE 0x800

/** Size of a FAT page */
#define FAT_PAGE_SZ 0x1000L

/** Lock the volume */
#define FATX_LOCK(x) pthread_mutex_lock(&(x)->devLock)
#define FATX_UNLOCK(x) pthread_mutex_unlock(&(x)->devLock)

/** FAT page cache */
typedef struct fatx_fat_cache_entry {
	/** The FAT page number */
	uint32_t pageNo;
	/** Whether this fat page has been written to. */
	char     dirty;
	/** The actual FAT entries */
	union {
		char     data[FAT_PAGE_SZ];
		uint32_t fatx32Entries[FATX32_ENTRIES_PER_PAGE];
		uint16_t fatx16Entries[FATX16_ENTRIES_PER_PAGE];
	};
} fatx_fat_cache_entry;

/** Internal cache entry structure */
typedef struct fatx_cache_entry {
	/** The cluster number of this entry */
	uint32_t			clusterNo;
	/** Dirty flag */
	char				dirty;
	/** Pointer to the actual data */
	char 				data[FAT_CLUSTER_SZ];
} fatx_cache_entry;

/** Internal fatx structure */
typedef struct fatx_handle {
   /** File descriptor of the device */
   int             		dev; 
	/** Mutex attributes */
	pthread_mutexattr_t  mutexAttr;
   /** Lock to synchronize access to the device */
   pthread_mutex_t 		devLock; 
   /** Number of clusters in the partition */
   uint32_t        		nClusters; 
   /** FAT type, either fat16 or fat32 */
   enum FAT_TYPE  	 	fatType; 
	/** Offset to the start of the data. */
	off_t				 		dataStart;
	/** Cache table */
	fatx_cache_entry 	   cache[CACHE_SIZE];
	/** FAT cache */
	fatx_fat_cache_entry fatCache[FAT_CACHE_SIZE];
} fatx_handle;

/** Filename linked list */
typedef struct fatx_filename_list {
	/** Filename. Limited to 42 characters. */
	char					          filename[43];
	struct fatx_filename_list * next;
} fatx_filename_list;

/** fatx_dirent_t linked list */
typedef struct fatx_dirent_list {
	/** This directory entry. */
	fatx_dirent_t *           dirEnt;
	/** Next dirent in the chain */
	struct fatx_dirent_list * next;
} fatx_dirent_list;

/** Directory iterator */
typedef struct fatx_dir_iter {
	/** Reference to the fatx object this dir iter is associated with */
	fatx_handle * 			fatx_h;
	/** Cluster number in the folder */
	uint32_t				   clusterNo;
	/** Entry number */
	uint32_t				   entryNo;
	/** List of dirents given out */
	fatx_dirent_list *   dirEntList;
} fatx_dir_iter;

/** FATX directory entry */
typedef struct fatx_directory_entry {
	/** Length of the file name */
	uint8_t					filenameSz;
	/** FAT Attributes */
	uint8_t					attributes;
	/** Filename */
	char                 filename[42];
	/** First cluster of the file */
	uint32_t					firstCluster;
	/** File size */
	uint32_t					fileSize;
	/** Modification time */
	uint16_t					modificationTime;
	/** Modification date */
	uint16_t					modificationDate;
	/** Creation time */
	uint16_t					creationTime;
	/** Creation date */
	uint16_t					creationDate;
	/** Last access time */
	uint16_t					accessTime;
	/** Last access date */
	uint16_t					accessDate;
} fatx_directory_entry;

/**
 * Calculate the number of clusters in a fatx device.
 *
 * \param dev fd of the device to check.
 * \return number of clusters in the fatx device.
 */
uint32_t fatx_calcClusters(int dev);

/**
 * Calculate the start of the data clusters.
 *
 * \param fatType the type of fat, 1 for FATX16, 2 for FATX32
 * \param clusters number of clusters.
 * \return the starting offset of the first data cluster
 */
off_t fatx_calcDataStart(int fatType, uint32_t clusters);

/**
 * Retrieve a FAT entry
 *
 * \param fatx_h the fatx object.
 * \param entryNo the entry to retrieve.
 * \return fatx entry
 */
uint32_t fatx_readFatEntry(fatx_handle * fatx_h, uint32_t entryNo);

/**
 * Is cluster then end of chain
 *
 * \param fatx_h the fatx object.
 * \param clusterNo number of the cluster.
 */
char fatx_isEOC(fatx_handle * fatx_h, uint32_t clusterNo);

/**
 * Load up a FAT page
 *
 * \param fatx_h the fatx object.
 * \param pageNo FAT page to load into the cache.
 */
void fatx_loadFatPage(fatx_handle * fatx_h, uint32_t pageNo);

/**
 * Write a fat page out to disk
 *
 * \param fatx_h the fatx object.
 * \param pageNo the FAT page to write out.
 */
void fatx_flushFatPage(fatx_handle * fatx_h, uint32_t pageNo);

/**
 * Get a cached cluster
 *
 * \param fatx_h the fatx object.
 * \param clusterNo cluster number to get.
 * \return cache entry corresponding to the requested cluster.
 */
fatx_cache_entry * fatx_getCluster(fatx_handle * fatx_h, uint32_t clusterNo);

/**
 * Read a cluster from disk
 *
 * \param fatx_h the fatx object.
 * \param clusterNo cluster to read.
 */
void fatx_loadCluster(fatx_handle * fatx_h, uint32_t clusterNo);

/**
 * Write a cluster out to disk
 *
 * \param fatx_h the fatx object.
 * \param clusterNo the cluster to flush.
 */
void fatx_flushCluster(fatx_handle * fatx_h, uint32_t clusterNo);

/**
 * Free a filename list
 *
 * \param fnList Filename list to free.
 */
void fatx_freeFilenameList(fatx_filename_list * fnList);

/**
 * Split a path.
 *
 * \param path the path to split.
 * \return list of the path pieces.
 */
fatx_filename_list * fatx_splitPath(const char * path);

/**
 * Create a dir iterator.
 *
 * \param fatx_h the fatx object.
 * \param fnList path segment list.
 * \param baseDir the dir iterator of the directory to start looking from.
 * \return the dir iterator.
 */
fatx_dir_iter * fatx_createDirIter(fatx_handle * fatx_h, 
											  fatx_filename_list * fnList, fatx_dir_iter * baseDir);
#endif // __LIBFATX_INTERNAL_H__
