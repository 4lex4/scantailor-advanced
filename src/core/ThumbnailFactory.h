// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_THUMBNAILFACTORY_H_
#define SCANTAILOR_CORE_THUMBNAILFACTORY_H_

#include <QSizeF>
#include <memory>

#include "NonCopyable.h"
#include "ThumbnailPixmapCache.h"

class PageInfo;
class CompositeCacheDrivenTask;
class QGraphicsItem;

class ThumbnailFactory {
  DECLARE_NON_COPYABLE(ThumbnailFactory)

 public:
  ThumbnailFactory(std::shared_ptr<ThumbnailPixmapCache> pixmapCache,
                   const QSizeF& maxSize,
                   std::shared_ptr<CompositeCacheDrivenTask> task);

  virtual ~ThumbnailFactory();

  std::unique_ptr<QGraphicsItem> get(const PageInfo& pageInfo);

 private:
  class Collector;

  std::shared_ptr<ThumbnailPixmapCache> m_pixmapCache;
  QSizeF m_maxSize;
  std::shared_ptr<CompositeCacheDrivenTask> m_task;
};


#endif  // ifndef SCANTAILOR_CORE_THUMBNAILFACTORY_H_
