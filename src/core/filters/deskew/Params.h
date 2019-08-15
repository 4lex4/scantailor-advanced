/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef DESKEW_PARAMS_H_
#define DESKEW_PARAMS_H_

#include <QString>
#include <algorithm>
#include <cmath>
#include "AutoManualMode.h"
#include "Dependencies.h"

class QDomDocument;
class QDomElement;

namespace deskew {
class Params {
 public:
  // Member-wise copying is OK.

  Params(double deskew_angle_deg, const Dependencies& deps, AutoManualMode mode);

  explicit Params(const QDomElement& deskew_el);

  ~Params();

  double deskewAngle() const;

  const Dependencies& dependencies() const;

  AutoManualMode mode() const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  double m_deskewAngleDeg;
  Dependencies m_deps;
  AutoManualMode m_mode;
};
}  // namespace deskew
#endif  // ifndef DESKEW_PARAMS_H_
