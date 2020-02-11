// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_CACHEDRIVENTASK_H_
#define SCANTAILOR_DESKEW_CACHEDRIVENTASK_H_

#include <memory>

#include "NonCopyable.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace select_content {
class CacheDrivenTask;
}

namespace deskew {
class Settings;

class CacheDrivenTask {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(std::shared_ptr<Settings> settings, std::shared_ptr<select_content::CacheDrivenTask> nextTask);

  virtual ~CacheDrivenTask();

  void process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector, const ImageTransformation& xform);

 private:
  std::shared_ptr<select_content::CacheDrivenTask> m_nextTask;
  std::shared_ptr<Settings> m_settings;
};
}  // namespace deskew
#endif  // ifndef SCANTAILOR_DESKEW_CACHEDRIVENTASK_H_
