// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_THUMBNAILFACTORY_H_
#define SCANTAILOR_CORE_THUMBNAILFACTORY_H_

#include <QSizeF>
#include <memory>

#include "NonCopyable.h"
#include "ThumbnailPixmapCache.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class PageInfo;
class CompositeCacheDrivenTask;
class QGraphicsItem;

class ThumbnailFactory : public ref_countable {
  DECLARE_NON_COPYABLE(ThumbnailFactory)

 public:
  ThumbnailFactory(intrusive_ptr<ThumbnailPixmapCache> pixmapCache,
                   const QSizeF& maxSize,
                   intrusive_ptr<CompositeCacheDrivenTask> task);

  ~ThumbnailFactory() override;

  std::unique_ptr<QGraphicsItem> get(const PageInfo& pageInfo);

 private:
  class Collector;

  intrusive_ptr<ThumbnailPixmapCache> m_pixmapCache;
  QSizeF m_maxSize;
  intrusive_ptr<CompositeCacheDrivenTask> m_task;
};


#endif  // ifndef SCANTAILOR_CORE_THUMBNAILFACTORY_H_
