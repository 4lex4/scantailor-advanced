// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BadAllocIfNull.h"

#include <QImage>
#include <new>

namespace imageproc {
const QImage& badAllocIfNull(const QImage& image) {
  if (image.isNull()) {
    throw std::bad_alloc();
  }
  return image;
}
}  // namespace imageproc
