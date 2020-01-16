// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageView.h"

#include "ImagePresentation.h"

namespace fix_orientation {
ImageView::ImageView(const QImage& image, const QImage& downscaledImage, const ImageTransformation& xform)
    : ImageViewBase(image, downscaledImage, ImagePresentation(xform.transform(), xform.resultingPreCropArea())),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_xform(xform) {
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView() = default;

void ImageView::setPreRotation(const OrthogonalRotation rotation) {
  if (m_xform.preRotation() == rotation) {
    return;
  }

  m_xform.setPreRotation(rotation);

  // This should call update() by itself.
  updateTransform(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}
}  // namespace fix_orientation
