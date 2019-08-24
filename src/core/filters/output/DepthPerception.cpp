// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DepthPerception.h"
#include <foundation/Utils.h>

using namespace foundation;

namespace output {
DepthPerception::DepthPerception() : m_value(defaultValue()) {}

DepthPerception::DepthPerception(double value) : m_value(qBound(minValue(), value, maxValue())) {}

DepthPerception::DepthPerception(const QString& fromString) {
  bool ok = false;
  m_value = fromString.toDouble(&ok);
  if (!ok) {
    m_value = defaultValue();
  } else {
    m_value = qBound(minValue(), m_value, maxValue());
  }
}

QString DepthPerception::toString() const {
  return Utils::doubleToString(m_value);
}

void DepthPerception::setValue(double value) {
  m_value = qBound(minValue(), value, maxValue());
}

double DepthPerception::value() const {
  return m_value;
}

double DepthPerception::minValue() {
  return 1.0;
}

double DepthPerception::defaultValue() {
  return 2.0;
}

double DepthPerception::maxValue() {
  return 3.0;
}
}  // namespace output