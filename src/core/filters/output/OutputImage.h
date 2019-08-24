// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OUTPUTIMAGE_H_
#define SCANTAILOR_OUTPUT_OUTPUTIMAGE_H_

#include <QImage>

class Dpm;

namespace output {
class OutputImage {
 public:
  virtual ~OutputImage() = default;

  virtual QImage toImage() const = 0;

  virtual bool isNull() const = 0;

  virtual void setDpm(const Dpm& dpm) = 0;

  virtual operator QImage() const { return toImage(); }
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUT_OUTPUTIMAGE_H_
