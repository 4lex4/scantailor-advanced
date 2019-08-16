// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef FIX_ORIENTATION_CACHEDRIVENTASK_H_
#define FIX_ORIENTATION_CACHEDRIVENTASK_H_

#include "CompositeCacheDrivenTask.h"
#include "NonCopyable.h"
#include "intrusive_ptr.h"

class PageInfo;
class AbstractFilterDataCollector;

namespace page_split {
class CacheDrivenTask;
}

namespace fix_orientation {
class Settings;

class CacheDrivenTask : public CompositeCacheDrivenTask {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<page_split::CacheDrivenTask> next_task);

  ~CacheDrivenTask() override;

  void process(const PageInfo& page_info, AbstractFilterDataCollector* collector) override;

 private:
  intrusive_ptr<page_split::CacheDrivenTask> m_nextTask;
  intrusive_ptr<Settings> m_settings;
};
}  // namespace fix_orientation
#endif  // ifndef FIX_ORIENTATION_CACHEDRIVENTASK_H_
