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

#include "Params.h"
#include <QDomDocument>
#include "../../Utils.h"

namespace deskew {
Params::Params(const double deskew_angle_deg, const Dependencies& deps, const AutoManualMode mode)
    : m_deskewAngleDeg(deskew_angle_deg), m_deps(deps), m_mode(mode) {}

Params::Params(const QDomElement& deskew_el)
    : m_deskewAngleDeg(deskew_el.attribute("angle").toDouble()),
      m_deps(deskew_el.namedItem("dependencies").toElement()),
      m_mode(deskew_el.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO) {}

Params::~Params() = default;

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("mode", m_mode == MODE_AUTO ? "auto" : "manual");
  el.setAttribute("angle", Utils::doubleToString(m_deskewAngleDeg));
  el.appendChild(m_deps.toXml(doc, "dependencies"));

  return el;
}

double Params::deskewAngle() const {
  return m_deskewAngleDeg;
}
const Dependencies& Params::dependencies() const {
  return m_deps;
}

AutoManualMode Params::mode() const {
  return m_mode;
}
}  // namespace deskew