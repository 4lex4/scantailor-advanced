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

#ifndef THUMBNAILFACTORY_H_
#define THUMBNAILFACTORY_H_

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
  ThumbnailFactory(intrusive_ptr<ThumbnailPixmapCache> pixmap_cache,
                   const QSizeF& max_size,
                   intrusive_ptr<CompositeCacheDrivenTask> task);

  ~ThumbnailFactory() override;

  std::unique_ptr<QGraphicsItem> get(const PageInfo& page_info);

 private:
  class Collector;

  intrusive_ptr<ThumbnailPixmapCache> m_ptrPixmapCache;
  QSizeF m_maxSize;
  intrusive_ptr<CompositeCacheDrivenTask> m_ptrTask;
};


#endif  // ifndef THUMBNAILFACTORY_H_
