// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DESKEW_THUMBNAIL_H_
#define DESKEW_THUMBNAIL_H_

#include "ThumbnailBase.h"
#include "intrusive_ptr.h"

class QSizeF;
class ThumbnailPixmapCache;
class ImageId;
class ImageTransformation;

namespace deskew {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
            const QSizeF& max_size,
            const ImageId& image_id,
            const ImageTransformation& xform,
            bool deviant = false);

  void prePaintOverImage(QPainter& painter,
                         const QTransform& image_to_display,
                         const QTransform& thumb_to_display) override;

 private:
  bool m_deviant;
};
}  // namespace deskew
#endif
