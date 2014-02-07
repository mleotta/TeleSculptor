/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef MAPTK_LANDMARK_MAP_IO_H_
#define MAPTK_LANDMARK_MAP_IO_H_

#include "core_config.h"

#include "landmark_map.h"

/**
 * \file
 * \brief File IO functions for a \ref maptk::landmark_map
 *
 * Uses the PLY file format
 */

namespace maptk
{


/// Output the given \c landmark_map object to the specified PLY file path
/**
 * If a file exists at the target location, it will be overwritten. If the
 * containing directory of the given path does not exist, it will be created
 * before the file is opened for writing.
 *
 * \throws file_write_exception
 *    Thrown when something prevents output of the file.
 * \throws boost::filesystem::filesystem:error
 *    Thrown when an underlying boost::filesystem call failes for system
 *    reasons.
 *
 * \param landmarks The \c landmark_map object to output.
 * \param file_path The path to output the file to.
 */
void
MAPTK_CORE_EXPORT
write_ply_file(landmark_map_sptr const& landmarks,
               path_t const& file_path);

}

#endif // MAPTK_LANDMARK_MAP_IO_H_