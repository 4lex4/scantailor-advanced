// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef BASICIMAGEVIEW_H_
#define BASICIMAGEVIEW_H_

#include <QImage>
#include "DragHandler.h"
#include "ImagePixmapUnion.h"
#include "ImageViewBase.h"
#include "Margins.h"
#include "ZoomHandler.h"

class BasicImageView : public ImageViewBase {
  Q_OBJECT
 public:
  explicit BasicImageView(const QImage& image,
                          const ImagePixmapUnion& downscaled_image = ImagePixmapUnion(),
                          const Margins& margins = Margins());

  ~BasicImageView() override;

 private:
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
};


#endif
