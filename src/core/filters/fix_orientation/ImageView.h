// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_IMAGEVIEW_H_
#define SCANTAILOR_FIX_ORIENTATION_IMAGEVIEW_H_

#include "DragHandler.h"
#include "ImageTransformation.h"
#include "ImageViewBase.h"
#include "OrthogonalRotation.h"
#include "ZoomHandler.h"

namespace fix_orientation {
class ImageView : public ImageViewBase {
  Q_OBJECT
 public:
  ImageView(const QImage& image, const QImage& downscaledImage, const ImageTransformation& xform);

  ~ImageView() override;

 public slots:

  void setPreRotation(OrthogonalRotation rotation);

 private:
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
  ImageTransformation m_xform;
};
}  // namespace fix_orientation
#endif
