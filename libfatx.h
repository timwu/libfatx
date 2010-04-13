/**
 * \file libfatx.h
 * \author Tim Wu
 */

#ifndef FATX_LIBFATX_H
#define FATX_LIBFATX_H

#include "dirent.h"

/**
 * A fatx opaque object.
 */
typedef fatx_handle *fatx_t;

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

#endif //FATX_LIBFATX_H
