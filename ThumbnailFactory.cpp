/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ThumbnailFactory.h"
#include <QGraphicsItem>
#include <utility>
#include "CompositeCacheDrivenTask.h"
#include "filter_dc/ThumbnailCollector.h"

class ThumbnailFactory::Collector : public ThumbnailCollector {
 public:
  Collector(intrusive_ptr<ThumbnailPixmapCache> cache, const QSizeF& max_size);

  void processThumbnail(std::unique_ptr<QGraphicsItem> thumbnail) override;

  intrusive_ptr<ThumbnailPixmapCache> thumbnailCache() override;

  QSizeF maxLogicalThumbSize() const override;

  std::unique_ptr<QGraphicsItem> retrieveThumbnail() { return std::move(m_ptrThumbnail); }

 private:
  intrusive_ptr<ThumbnailPixmapCache> m_ptrCache;
  QSizeF m_maxSize;
  std::unique_ptr<QGraphicsItem> m_ptrThumbnail;
};


ThumbnailFactory::ThumbnailFactory(intrusive_ptr<ThumbnailPixmapCache> pixmap_cache,
                                   const QSizeF& max_size,
                                   intrusive_ptr<CompositeCacheDrivenTask> task)
    : m_ptrPixmapCache(std::move(pixmap_cache)), m_maxSize(max_size), m_ptrTask(std::move(task)) {}

ThumbnailFactory::~ThumbnailFactory() = default;

std::unique_ptr<QGraphicsItem> ThumbnailFactory::get(const PageInfo& page_info) {
  Collector collector(m_ptrPixmapCache, m_maxSize);
  m_ptrTask->process(page_info, &collector);

  return collector.retrieveThumbnail();
}

/*======================= ThumbnailFactory::Collector ======================*/

ThumbnailFactory::Collector::Collector(intrusive_ptr<ThumbnailPixmapCache> cache, const QSizeF& max_size)
    : m_ptrCache(std::move(cache)), m_maxSize(max_size) {}

void ThumbnailFactory::Collector::processThumbnail(std::unique_ptr<QGraphicsItem> thumbnail) {
  m_ptrThumbnail = std::move(thumbnail);
}

intrusive_ptr<ThumbnailPixmapCache> ThumbnailFactory::Collector::thumbnailCache() {
  return m_ptrCache;
}

QSizeF ThumbnailFactory::Collector::maxLogicalThumbSize() const {
  return m_maxSize;
}
