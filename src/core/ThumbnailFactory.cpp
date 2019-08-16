// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ThumbnailFactory.h"
#include <QGraphicsItem>
#include <utility>
#include "CompositeCacheDrivenTask.h"
#include "ThumbnailCollector.h"

class ThumbnailFactory::Collector : public ThumbnailCollector {
 public:
  Collector(intrusive_ptr<ThumbnailPixmapCache> cache, const QSizeF& max_size);

  void processThumbnail(std::unique_ptr<QGraphicsItem> thumbnail) override;

  intrusive_ptr<ThumbnailPixmapCache> thumbnailCache() override;

  QSizeF maxLogicalThumbSize() const override;

  std::unique_ptr<QGraphicsItem> retrieveThumbnail() { return std::move(m_thumbnail); }

 private:
  intrusive_ptr<ThumbnailPixmapCache> m_cache;
  QSizeF m_maxSize;
  std::unique_ptr<QGraphicsItem> m_thumbnail;
};


ThumbnailFactory::ThumbnailFactory(intrusive_ptr<ThumbnailPixmapCache> pixmap_cache,
                                   const QSizeF& max_size,
                                   intrusive_ptr<CompositeCacheDrivenTask> task)
    : m_pixmapCache(std::move(pixmap_cache)), m_maxSize(max_size), m_task(std::move(task)) {}

ThumbnailFactory::~ThumbnailFactory() = default;

std::unique_ptr<QGraphicsItem> ThumbnailFactory::get(const PageInfo& page_info) {
  Collector collector(m_pixmapCache, m_maxSize);
  m_task->process(page_info, &collector);

  return collector.retrieveThumbnail();
}

/*======================= ThumbnailFactory::Collector ======================*/

ThumbnailFactory::Collector::Collector(intrusive_ptr<ThumbnailPixmapCache> cache, const QSizeF& max_size)
    : m_cache(std::move(cache)), m_maxSize(max_size) {}

void ThumbnailFactory::Collector::processThumbnail(std::unique_ptr<QGraphicsItem> thumbnail) {
  m_thumbnail = std::move(thumbnail);
}

intrusive_ptr<ThumbnailPixmapCache> ThumbnailFactory::Collector::thumbnailCache() {
  return m_cache;
}

QSizeF ThumbnailFactory::Collector::maxLogicalThumbSize() const {
  return m_maxSize;
}
