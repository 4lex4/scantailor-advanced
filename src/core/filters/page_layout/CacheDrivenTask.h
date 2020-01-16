// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_CACHEDRIVENTASK_H_
#define SCANTAILOR_PAGE_LAYOUT_CACHEDRIVENTASK_H_

#include <QPolygonF>

#include "NonCopyable.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QRectF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace output {
class CacheDrivenTask;
}

namespace page_layout {
class Settings;

class CacheDrivenTask : public ref_countable {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<output::CacheDrivenTask> nextTask, intrusive_ptr<Settings> settings);

  ~CacheDrivenTask() override;

  void process(const PageInfo& pageInfo,
               AbstractFilterDataCollector* collector,
               const ImageTransformation& xform,
               const QRectF& pageRect,
               const QRectF& contentRect);

 private:
  intrusive_ptr<output::CacheDrivenTask> m_nextTask;
  intrusive_ptr<Settings> m_settings;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_CACHEDRIVENTASK_H_
