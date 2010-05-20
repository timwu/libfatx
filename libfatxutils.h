/**
 * \file libfatxutils.h
 * \author Tim Wu
 */

/**
 * Calculate the number of clusters in a fatx device.
 *
 * \param dev fd of the device to check.
 * \return number of clusters in the fatx device.
 */
uint64_t fatx_calcClusters(int dev);
off_t fatx_calcFatSize(int entrySize, uint64_t clusters);
