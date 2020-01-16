// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageId.h"

#include <QFileInfo>

ImageId::ImageId(const QString& filePath, const int page) : m_filePath(filePath), m_page(page) {}

ImageId::ImageId(const QFileInfo& fileInfo, const int page) : m_filePath(fileInfo.absoluteFilePath()), m_page(page) {}

bool operator==(const ImageId& lhs, const ImageId& rhs) {
  return ((lhs.page() == rhs.page()) && (lhs.filePath() == rhs.filePath()));
}

bool operator!=(const ImageId& lhs, const ImageId& rhs) {
  return !(lhs == rhs);
}

bool operator<(const ImageId& lhs, const ImageId& rhs) {
  const int comp = lhs.filePath().compare(rhs.filePath());
  if (comp < 0) {
    return true;
  } else if (comp > 0) {
    return false;
  }
  return lhs.page() < rhs.page();
}
