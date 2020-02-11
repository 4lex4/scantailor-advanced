// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_CACHEDRIVENTASK_H_
#define SCANTAILOR_SELECT_CONTENT_CACHEDRIVENTASK_H_

#include <memory>

#include "NonCopyable.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace page_layout {
class CacheDrivenTask;
}

namespace select_content {
class Settings;

class CacheDrivenTask {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(std::shared_ptr<Settings> settings, std::shared_ptr<page_layout::CacheDrivenTask> nextTask);

  virtual ~CacheDrivenTask();

  void process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector, const ImageTransformation& xform);

 private:
  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<page_layout::CacheDrivenTask> m_nextTask;
};
}  // namespace select_content
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_CACHEDRIVENTASK_H_
