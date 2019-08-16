// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Dpm.h"
#include <QImage>
#include "Dpi.h"
#include <Constants.h>

Dpm::Dpm(const QSize size) : m_xDpm(size.width()), m_yDpm(size.height()) {}

Dpm::Dpm(const Dpi dpi)
    : m_xDpm(qRound(dpi.horizontal() * constants::DPI2DPM)), m_yDpm(qRound(dpi.vertical() * constants::DPI2DPM)) {}

Dpm::Dpm(const QImage& image) : m_xDpm(image.dotsPerMeterX()), m_yDpm(image.dotsPerMeterY()) {}

bool Dpm::isNull() const {
  return Dpi(*this).isNull();
}

QSize Dpm::toSize() const {
  if (isNull()) {
    return QSize();
  } else {
    return QSize(m_xDpm, m_yDpm);
  }
}

bool Dpm::operator==(const Dpm& other) const {
  return m_xDpm == other.m_xDpm && m_yDpm == other.m_yDpm;
}
