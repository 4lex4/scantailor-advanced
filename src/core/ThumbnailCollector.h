// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef THUMBNAILCOLLECTOR_H_
#define THUMBNAILCOLLECTOR_H_

#include <memory>
#include "AbstractFilterDataCollector.h"

class ThumbnailPixmapCache;
class QGraphicsItem;
class QSizeF;

class ThumbnailCollector : public AbstractFilterDataCollector {
 public:
  virtual void processThumbnail(std::unique_ptr<QGraphicsItem>) = 0;

  virtual intrusive_ptr<ThumbnailPixmapCache> thumbnailCache() = 0;

  virtual QSizeF maxLogicalThumbSize() const = 0;
};


#endif
