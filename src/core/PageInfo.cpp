// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageInfo.h"

PageInfo::PageInfo() : m_imageSubPages(0), m_leftHalfRemoved(false), m_rightHalfRemoved(false) {}

PageInfo::PageInfo(const PageId& page_id,
                   const ImageMetadata& metadata,
                   int image_sub_pages,
                   bool left_half_removed,
                   bool right_half_removed)
    : m_pageId(page_id),
      m_metadata(metadata),
      m_imageSubPages(image_sub_pages),
      m_leftHalfRemoved(left_half_removed),
      m_rightHalfRemoved(right_half_removed) {}
