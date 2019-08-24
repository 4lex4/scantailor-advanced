// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_CACHEDRIVENTASK_H_
#define SCANTAILOR_PAGE_SPLIT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;
class ProjectPages;

namespace deskew {
class CacheDrivenTask;
}

namespace page_split {
class Settings;

class CacheDrivenTask : public ref_countable {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings,
                  intrusive_ptr<ProjectPages> projectPages,
                  intrusive_ptr<deskew::CacheDrivenTask> nextTask);

  ~CacheDrivenTask() override;

  void process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector, const ImageTransformation& xform);

 private:
  intrusive_ptr<deskew::CacheDrivenTask> m_nextTask;
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ProjectPages> m_projectPages;
};
}  // namespace page_split
#endif  // ifndef SCANTAILOR_PAGE_SPLIT_CACHEDRIVENTASK_H_
