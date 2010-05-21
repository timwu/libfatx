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

/**
 * Calculate the start of the data clusters.
 *
 * \param fatType the type of fat, 1 for FATX16, 2 for FATX32
 * \param clusters number of clusters.
 * \return the starting offset of the first data cluster
 */
off_t fatx_calcFatSize(int fatType, uint64_t clusters);
