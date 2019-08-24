// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_CONTENTBOXCOLLECTOR_H_
#define SCANTAILOR_CORE_CONTENTBOXCOLLECTOR_H_

#include "AbstractFilterDataCollector.h"

class ImageTransformation;
class QRectF;

class ContentBoxCollector : public AbstractFilterDataCollector {
 public:
  virtual void process(const ImageTransformation& xform, const QRectF& contentRect) = 0;
};


#endif
