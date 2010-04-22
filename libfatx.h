/**
 * \file libfatx.h
 * \author Tim Wu
 */

#include <sys/stat.h>

#ifndef FATX_LIBFATX_H
#define FATX_LIBFATX_H

/**
 * A fatx opaque object.
 */
typedef struct fatx_handle * fatx_t;

#include "dirent.h"

/**
 * Initializes a fatx opaque object with the path to
 * the device to use.
 *
 * \param path Path to the image or device.
 * \return The initilized fatx object; NULL on error.
 */
fatx_t fatx_init(const char* path);

/**
 * Frees a fatx object.
 *
 * \param fatx The fatx object to be freed.
 */
void fatx_free(fatx_t fatx);

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
 
#endif //FATX_LIBFATX_H
