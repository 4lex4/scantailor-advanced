// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_CACHEDRIVENTASK_H_
#define SCANTAILOR_OUTPUT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "OutputFileNameGenerator.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QPolygonF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace output {
class Settings;

class CacheDrivenTask : public ref_countable {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings, const OutputFileNameGenerator& outFileNameGen);

  ~CacheDrivenTask() override;

  void process(const PageInfo& pageInfo,
               AbstractFilterDataCollector* collector,
               const ImageTransformation& xform,
               const QPolygonF& contentRectPhys);

 private:
  intrusive_ptr<Settings> m_settings;
  OutputFileNameGenerator m_outFileNameGen;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_CACHEDRIVENTASK_H_
