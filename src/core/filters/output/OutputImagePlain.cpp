// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputImagePlain.h"

#include <imageproc/Dpm.h>

namespace output {
OutputImagePlain::OutputImagePlain(const QImage& image) : m_image(image) {}

QImage OutputImagePlain::toImage() const {
  return m_image;
}

bool OutputImagePlain::isNull() const {
  return m_image.isNull();
}

void OutputImagePlain::setDpm(const Dpm& dpm) {
  m_image.setDotsPerMeterX(dpm.horizontal());
  m_image.setDotsPerMeterY(dpm.vertical());
}
}  // namespace output