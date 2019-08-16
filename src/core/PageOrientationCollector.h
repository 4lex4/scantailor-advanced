// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_ORIENTATION_COLLECTOR_H_
#define PAGE_ORIENTATION_COLLECTOR_H_

#include "AbstractFilterDataCollector.h"

class OrthogonalRotation;

class PageOrientationCollector : public AbstractFilterDataCollector {
 public:
  virtual void process(const OrthogonalRotation& orientation) = 0;
};


#endif
