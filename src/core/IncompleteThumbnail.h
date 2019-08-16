// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef INCOMPLETETHUMBNAIL_H_
#define INCOMPLETETHUMBNAIL_H_

#include <QPainterPath>
#include "ThumbnailBase.h"
#include "intrusive_ptr.h"

class ThumbnailPixmapCache;
class QSizeF;
class QRectF;

/**
 * \brief A thumbnail represeting a page not completely processed.
 *
 * Suppose you have switched to the Deskew filter without running all images
 * through the previous filters.  The thumbnail view of the Deskew filter
 * is supposed to show already split and deskewed pages.  However, because
 * the previous filters were not applied to all pages, we can't show them
 * split and deskewed.  In that case, we show an image the best we can
 * (maybe split but not deskewed, maybe unsplit).  Also we draw a translucent
 * question mark over that image to indicate it's not shown the way it should.
 * This class implements drawing of such thumbnails with question marks.
 */
class IncompleteThumbnail : public ThumbnailBase {
 public:
  IncompleteThumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                      const QSizeF& max_size,
                      const ImageId& image_id,
                      const ImageTransformation& image_xform);

  ~IncompleteThumbnail() override;

  static void drawQuestionMark(QPainter& painter, const QRectF& bounding_rect);

 protected:
  void paintOverImage(QPainter& painter,
                      const QTransform& image_to_display,
                      const QTransform& thumb_to_display) override;

 private:
  static QPainterPath m_sCachedPath;
};


#endif
