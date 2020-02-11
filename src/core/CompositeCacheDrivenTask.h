// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_COMPOSITECACHEDRIVENTASK_H_
#define SCANTAILOR_CORE_COMPOSITECACHEDRIVENTASK_H_


class PageInfo;
class AbstractFilterDataCollector;

class CompositeCacheDrivenTask {
 public:
  virtual ~CompositeCacheDrivenTask() = default;

  virtual void process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector) = 0;
};


#endif
