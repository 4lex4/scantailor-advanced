// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageView.h"
#include "ImagePresentation.h"
#include "OutputMargins.h"

namespace output {
ImageView::ImageView(const QImage& image, const QImage& downscaled_image)
    : ImageViewBase(image, downscaled_image, ImagePresentation(QTransform(), QRectF(image.rect())), OutputMargins()),
      m_dragHandler(*this),
      m_zoomHandler(*this) {
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView() = default;
}  // namespace output
