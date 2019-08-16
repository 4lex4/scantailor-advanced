// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEFILEINFO_H_
#define IMAGEFILEINFO_H_

#include <QFileInfo>
#include <vector>
#include "ImageMetadata.h"

class ImageFileInfo {
  // Member-wise copying is OK.
 public:
  ImageFileInfo(const QFileInfo& file_info, const std::vector<ImageMetadata>& image_info)
      : m_fileInfo(file_info), m_imageInfo(image_info) {}

  const QFileInfo& fileInfo() const { return m_fileInfo; }

  std::vector<ImageMetadata>& imageInfo() { return m_imageInfo; }

  const std::vector<ImageMetadata>& imageInfo() const { return m_imageInfo; }

  bool isDpiOK() const;

 private:
  QFileInfo m_fileInfo;
  std::vector<ImageMetadata> m_imageInfo;
};


#endif  // ifndef IMAGEFILEINFO_H_
