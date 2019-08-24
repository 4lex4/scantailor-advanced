// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageInfo.h"

PageInfo::PageInfo() : m_imageSubPages(0), m_leftHalfRemoved(false), m_rightHalfRemoved(false) {}

PageInfo::PageInfo(const PageId& pageId,
                   const ImageMetadata& metadata,
                   int imageSubPages,
                   bool leftHalfRemoved,
                   bool rightHalfRemoved)
    : m_pageId(pageId),
      m_metadata(metadata),
      m_imageSubPages(imageSubPages),
      m_leftHalfRemoved(leftHalfRemoved),
      m_rightHalfRemoved(rightHalfRemoved) {}
