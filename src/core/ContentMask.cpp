// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ContentMask.h"

#include <imageproc/Binarize.h>
#include <imageproc/Dpi.h>
#include <imageproc/GrayImage.h>
#include <imageproc/PolygonRasterizer.h>
#include <imageproc/Transform.h>

#include <cmath>

#include "Despeckle.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"

using namespace imageproc;

ContentMask::ContentMask(const GrayImage& grayImage, const ImageTransformation& xform, const TaskStatus& status) {
  ImageTransformation xform150dpi(xform);
  xform150dpi.preScaleToDpi(Dpi(150, 150));
  if (xform150dpi.resultingRect().toRect().isEmpty()) {
    return;
  }
  m_originalToContentXform = xform150dpi.transform();
  m_contentToOriginalXform = m_originalToContentXform.inverted();

  QImage gray150(transformToGray(grayImage, xform150dpi.transform(), xform150dpi.resultingRect().toRect(),
                                 OutsidePixels::assumeColor(Qt::white)));
  m_image = binarizeWolf(gray150, QSize(51, 51), 50);
  PolygonRasterizer::fillExcept(m_image, WHITE, xform150dpi.resultingPreCropArea(), Qt::WindingFill);
  Despeckle::despeckleInPlace(m_image, Dpi(150, 150), Despeckle::NORMAL, status);
}

QRect ContentMask::findContentInArea(const QRect& area) const {
  if (isNull())
    return QRect();

  QRect findingArea = area.intersected(m_image.rect());
  if (findingArea.isEmpty())
    return QRect();

  const uint32_t* imageLine = m_image.data();
  const int imageStride = m_image.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  int top = std::numeric_limits<int>::max();
  int left = std::numeric_limits<int>::max();
  int bottom = std::numeric_limits<int>::min();
  int right = std::numeric_limits<int>::min();

  imageLine += findingArea.top() * imageStride;
  for (int y = findingArea.top(); y <= findingArea.bottom(); ++y) {
    for (int x = findingArea.left(); x <= findingArea.right(); ++x) {
      if (imageLine[x >> 5] & (msb >> (x & 31))) {
        top = std::min(top, y);
        left = std::min(left, x);
        bottom = std::max(bottom, y);
        right = std::max(right, x);
      }
    }
    imageLine += imageStride;
  }

  if (top > bottom) {
    return QRect();
  }

  QRect foundArea = QRect(left, top, right - left + 1, bottom - top + 1);
  foundArea.adjust(-1, -1, 1, 1);
  return foundArea;
}
