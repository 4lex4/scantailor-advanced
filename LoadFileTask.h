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

#ifndef LOADFILETASK_H_
#define LOADFILETASK_H_

#include "BackgroundTask.h"
#include "FilterResult.h"
#include "ImageId.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "intrusive_ptr.h"

class ThumbnailPixmapCache;
class PageInfo;
class ProjectPages;
class QImage;

namespace fix_orientation {
class Task;
}

class LoadFileTask : public BackgroundTask {
  DECLARE_NON_COPYABLE(LoadFileTask)

 public:
  LoadFileTask(Type type,
               const PageInfo& page,
               intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
               intrusive_ptr<ProjectPages> pages,
               intrusive_ptr<fix_orientation::Task> next_task);

  ~LoadFileTask() override;

  FilterResultPtr operator()() override;

 private:
  class ErrorResult;

  void updateImageSizeIfChanged(const QImage& image);

  void overrideDpi(QImage& image) const;

  intrusive_ptr<ThumbnailPixmapCache> m_ptrThumbnailCache;
  ImageId m_imageId;
  ImageMetadata m_imageMetadata;
  const intrusive_ptr<ProjectPages> m_ptrPages;
  const intrusive_ptr<fix_orientation::Task> m_ptrNextTask;
};


#endif  // ifndef LOADFILETASK_H_
