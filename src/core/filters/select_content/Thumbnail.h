// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_THUMBNAIL_H_
#define SCANTAILOR_SELECT_CONTENT_THUMBNAIL_H_

#include <QRectF>
#include "ThumbnailBase.h"

class QSizeF;
class ThumbnailPixmapCache;
class ImageId;
class ImageTransformation;

namespace select_content {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnailCache,
            const QSizeF& maxSize,
            const ImageId& imageId,
            const ImageTransformation& xform,
            const QRectF& contentRect,
            const QRectF& pageRect,
            bool pageRectEnabled,
            bool deviant);

  void paintOverImage(QPainter& painter, const QTransform& imageToDisplay, const QTransform& thumbToDisplay) override;

 private:
  QRectF m_contentRect;
  QRectF m_pageRect;
  bool m_pageRectEnabled;
  bool m_deviant;
};
}  // namespace select_content
#endif
