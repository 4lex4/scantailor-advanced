// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEINFO_H_
#define SCANTAILOR_CORE_IMAGEINFO_H_

#include "ImageId.h"
#include "ImageMetadata.h"

/**
 * This class stores the same information about an image as ProjectPages does,
 * and is used for adding images to ProjectPages objects.  Beyond that,
 * ProjectPages doesn't operate with ImageInfo objects, but with PageInfo ones.
 */
class ImageInfo {
  // Member-wise copying is OK.
 public:
  ImageInfo();

  ImageInfo(const ImageId& id,
            const ImageMetadata& metadata,
            int numSubPages,
            bool leftPageRemoved,
            bool rightPageRemoved);

  const ImageId& id() const { return m_id; }

  const ImageMetadata& metadata() const { return m_metadata; }

  int numSubPages() const { return m_numSubPages; }

  bool leftHalfRemoved() const { return m_leftHalfRemoved; }

  bool rightHalfRemoved() const { return m_rightHalfRemoved; }

 private:
  ImageId m_id;
  ImageMetadata m_metadata;
  int m_numSubPages;        // 1 or 2
  bool m_leftHalfRemoved;   // Both can't be true, and if one is true,
  bool m_rightHalfRemoved;  // then m_numSubPages is 1.
};


#endif  // ifndef SCANTAILOR_CORE_IMAGEINFO_H_
