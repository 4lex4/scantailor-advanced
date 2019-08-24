// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BasicImageView.h"
#include "Dpm.h"
#include "ImagePresentation.h"
#include "ImageTransformation.h"

BasicImageView::BasicImageView(const QImage& image, const ImagePixmapUnion& downscaledImage, const Margins& margins)
    : ImageViewBase(image, downscaledImage, ImagePresentation(QTransform(), QRectF(image.rect())), margins),
      m_dragHandler(*this),
      m_zoomHandler(*this) {
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

BasicImageView::~BasicImageView() = default;
