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

#ifndef DESKEW_TASK_H_
#define DESKEW_TASK_H_

#include <FilterData.h>
#include <memory>
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "ref_countable.h"

class TaskStatus;
class QImage;
class QSize;
class Dpi;
class DebugImages;

namespace imageproc {
class BinaryImage;
}

namespace select_content {
class Task;
}

namespace deskew {
class Filter;
class Settings;

class Task : public ref_countable {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(intrusive_ptr<Filter> filter,
       intrusive_ptr<Settings> settings,
       intrusive_ptr<ImageSettings> image_settings,
       intrusive_ptr<select_content::Task> next_task,
       const PageId& page_id,
       bool batch_processing,
       bool debug);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status, FilterData data);

 private:
  class UiUpdater;

  static void cleanup(const TaskStatus& status, imageproc::BinaryImage& img, const Dpi& dpi);

  static int from150dpi(int size, int target_dpi);

  static QSize from150dpi(const QSize& size, const Dpi& target_dpi);

  void updateFilterData(const TaskStatus& status, FilterData& data, bool needUpdate);

  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<Settings> m_ptrSettings;
  intrusive_ptr<ImageSettings> m_ptrImageSettings;
  intrusive_ptr<select_content::Task> m_ptrNextTask;
  std::unique_ptr<DebugImages> m_ptrDbg;
  PageId m_pageId;
  bool m_batchProcessing;
};
}  // namespace deskew
#endif  // ifndef DESKEW_TASK_H_
