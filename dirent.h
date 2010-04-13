/**
 * \file dirent.h
 * \author Tim Wu
 */

#ifndef FATX_DIRENT_H
#define FATX_DIRENT_H

#include "libfatx.h"

/**
 * Directory entry returned when iterating over folder.
 */
struct fatx_dirent {
  char d_name[]; /**< direntry name */
  unsigned short int d_namelen; /**< length of the name */
};

/**
 * Fatx dir opaque object used to iterate over
 * the contents of a directory.
 */
typedef fatx_dir_iter *fatx_dir_iter_t;

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
fatx_dirent* fatx_readdir(fatx_dir_iter_t iter); 

/**
 * Closes the iterator and frees up any associated structures.
 * 
 * \param iter Iterator to free.
 */
void fatx_closedir(fatx_dir_iter_t iter);

#endif //FATX_DIRENT_H
