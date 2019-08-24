// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OUTPUTIMAGEPLAIN_H_
#define SCANTAILOR_OUTPUT_OUTPUTIMAGEPLAIN_H_

#include "OutputImage.h"

namespace output {
class OutputImagePlain : public virtual OutputImage {
 public:
  OutputImagePlain() = default;

  explicit OutputImagePlain(const QImage& image);

  QImage toImage() const override;

  bool isNull() const override;

  void setDpm(const Dpm& dpm) override;

 private:
  QImage m_image;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUT_OUTPUTIMAGEPLAIN_H_
