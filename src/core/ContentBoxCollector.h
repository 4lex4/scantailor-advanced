// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef CONTENTBOXCOLLECTOR_H_
#define CONTENTBOXCOLLECTOR_H_

#include "AbstractFilterDataCollector.h"

class ImageTransformation;
class QRectF;

class ContentBoxCollector : public AbstractFilterDataCollector {
 public:
  virtual void process(const ImageTransformation& xform, const QRectF& content_rect) = 0;
};


#endif
