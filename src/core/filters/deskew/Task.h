// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_TASK_H_
#define SCANTAILOR_DESKEW_TASK_H_

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
       intrusive_ptr<ImageSettings> imageSettings,
       intrusive_ptr<select_content::Task> nextTask,
       const PageId& pageId,
       bool batchProcessing,
       bool debug);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status, FilterData data);

 private:
  class UiUpdater;

  static void cleanup(const TaskStatus& status, imageproc::BinaryImage& img, const Dpi& dpi);

  static int from150dpi(int size, int targetDpi);

  static QSize from150dpi(const QSize& size, const Dpi& targetDpi);

  void updateFilterData(const TaskStatus& status, FilterData& data, bool needUpdate);

  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  intrusive_ptr<select_content::Task> m_nextTask;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  bool m_batchProcessing;
};
}  // namespace deskew
#endif  // ifndef SCANTAILOR_DESKEW_TASK_H_
