// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECT_CONTENT_CACHEDRIVENTASK_H_
#define SELECT_CONTENT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace page_layout {
class CacheDrivenTask;
}

namespace select_content {
class Settings;

class CacheDrivenTask : public ref_countable {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<page_layout::CacheDrivenTask> next_task);

  ~CacheDrivenTask() override;

  void process(const PageInfo& page_info, AbstractFilterDataCollector* collector, const ImageTransformation& xform);

 private:
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<page_layout::CacheDrivenTask> m_nextTask;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_CACHEDRIVENTASK_H_
