// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGEINFO_H_
#define SCANTAILOR_CORE_PAGEINFO_H_

#include "ImageMetadata.h"
#include "PageId.h"

class PageInfo {
  // Member-wise copying is OK.
 public:
  PageInfo();

  PageInfo(const PageId& pageId,
           const ImageMetadata& metadata,
           int imageSubPages,
           bool leftHalfRemoved,
           bool rightHalfRemoved);

  bool isNull() const { return m_pageId.isNull(); }

  const PageId& id() const { return m_pageId; }

  void setId(const PageId& id) { m_pageId = id; }

  const ImageId& imageId() const { return m_pageId.imageId(); }

  const ImageMetadata& metadata() const { return m_metadata; }

  int imageSubPages() const { return m_imageSubPages; }

  bool leftHalfRemoved() const { return m_leftHalfRemoved; }

  bool rightHalfRemoved() const { return m_rightHalfRemoved; }

 private:
  PageId m_pageId;
  ImageMetadata m_metadata;
  int m_imageSubPages;
  bool m_leftHalfRemoved;
  bool m_rightHalfRemoved;
};


#endif  // ifndef SCANTAILOR_CORE_PAGEINFO_H_
