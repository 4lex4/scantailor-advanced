// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_IMAGEVIEW_H_
#define OUTPUT_IMAGEVIEW_H_

#include <QColor>
#include "DragHandler.h"
#include "ImageViewBase.h"
#include "ZoomHandler.h"

class ImageTransformation;

namespace output {
class ImageView : public ImageViewBase {
  Q_OBJECT
 public:
  ImageView(const QImage& image, const QImage& downscaled_image);

  ~ImageView() override;

 private:
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
};
}  // namespace output
#endif
