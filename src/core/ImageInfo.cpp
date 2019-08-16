// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageInfo.h"

ImageInfo::ImageInfo() : m_numSubPages(0), m_leftHalfRemoved(false), m_rightHalfRemoved(false) {}

ImageInfo::ImageInfo(const ImageId& id,
                     const ImageMetadata& metadata,
                     int num_sub_pages,
                     bool left_half_removed,
                     bool right_half_removed)
    : m_id(id),
      m_metadata(metadata),
      m_numSubPages(num_sub_pages),
      m_leftHalfRemoved(left_half_removed),
      m_rightHalfRemoved(right_half_removed) {}
