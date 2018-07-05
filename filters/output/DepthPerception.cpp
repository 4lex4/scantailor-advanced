/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "DepthPerception.h"
#include "../../Utils.h"

namespace output {
DepthPerception::DepthPerception() : m_value(defaultValue()) {}

DepthPerception::DepthPerception(double value) : m_value(qBound(minValue(), value, maxValue())) {}

DepthPerception::DepthPerception(const QString& from_string) {
  bool ok = false;
  m_value = from_string.toDouble(&ok);
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