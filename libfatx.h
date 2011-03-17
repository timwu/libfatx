/**
 * \file libfatx.h
 * \author Tim Wu
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef FATX_LIBFATX_H
#define FATX_LIBFATX_H

/**
 * A fatx opaque object.
 */
typedef struct fatx_handle * fatx_t;

/**
 * Directory entry returned when iterating over folder.
 */
typedef struct fatx_dirent {
  char * d_name; /**< direntry name */
  unsigned short int d_namelen; /**< length of the name */
} fatx_dirent_t;

/** Structure to store mount options */
typedef struct fatx_options {
   /** User to own the files */
   uid_t    user;
   /** Group of the files */
   gid_t    group;
   /** File permissions */
   uint32_t filePerm;
   /** Mount mode */
   uint32_t mode;
} fatx_options_t;

/**
 * Initializes a fatx opaque object with the path to
 * the device to use.
 *
 * \param path Path to the image or device.
 * \param options options to set.
 * \return The initilized fatx object; NULL on error.
 */
fatx_t fatx_init(const char* path, fatx_options_t * options);

/**
 * Frees a fatx object.
 *
 * \param fatx The fatx object to be freed.
 */
void fatx_free(fatx_t fatx);

/**
 * Print stats about a fatx_t object.
 *
 * \param fatx The fatx object to print info from.
 */
 void fatx_printInfo(fatx_t fatx);

/**
 * Read bytes from a file.
 *
 * \param fatx The fatx object
 * \param path Path to the file to be read.
 * \param buf Buffer to write read file data into.
 * \param offset Offset from the beginning of the file to start reading
 * \param size Number of bytes to read
 * \return The number of bytes written or an error.
 */
int fatx_read(fatx_t fatx, const char* path, char* buf, off_t offset, size_t size);

/**
 * Write bytes into a file
 *
 * \param fatx The fatx object
 * \param path The path to the file to be written to.
 * \param buf Buffer to read data from.
 * \param offset Offset in the file to write to.
 * \param size Number of bytes to write into the file.
 * \return The number of bytes written or an error
 */
int fatx_write(fatx_t fatx, const char* path, const char* buf, off_t offset, size_t size);

/**
 * Stat a file.
 *
 * \param fatx The fatx object.
 * \param path The path to the file to stat.
 * \param st_buf Pointer to the stat struct to populate.
 * \return Error code
 */
int fatx_stat(fatx_t fatx, const char* path, struct stat *st_buf);

/**
 * Remove a file
 *
 * \param fatx The fatx object.
 * \param path The path to the file to remove.
 * \return Error code
 */
int fatx_remove(fatx_t fatx, const char* path);

/**
 * Create a file
 *
 * \param fatx The fatx objects.
 * \param path Path to the file to create.
 * \return Error code
 */
int fatx_mkfile(fatx_t fatx, const char* path);

/**
 * Create a directory
 *
 * \param fatx The fatx object.
 * \param path Path to the directory to create.
 * \return Error code
 */
int fatx_mkdir(fatx_t fatx, const char* path);
 
/**
 * Fatx dir opaque object used to iterate over
 * the contents of a directory.
 */
typedef struct fatx_dir_iter * fatx_dir_iter_t;

/**
 * Opens a directory for reading. The resultant
 * iterator will be used to read directory entries
 * with the fatx_readdir() method. The iterator should
 * be freed with fatx_closedir() when no longer needed.
 *
 * \param fatx The fatx object
 * \param path Path of the directory to be opened.
 * \return Iterator to iterate over the directory entries; NULL on error.
 */
fatx_dir_iter_t fatx_opendir(fatx_t fatx, const char* path);

/**
 * Reads a directory entry from the given directory iterator.
 * 
 * \param iter The iterator to get the directory entries from.
 * \return A directory entry; NULL if none left.
 */
fatx_dirent_t * fatx_readdir(fatx_dir_iter_t iter); 

/**
 * Closes the iterator and frees up any associated structures.
 * 
 * \param iter Iterator to free.
 */
void fatx_closedir(fatx_dir_iter_t iter);

#endif //FATX_LIBFATX_H
