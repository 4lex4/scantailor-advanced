// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Params.h"

#include <foundation/Utils.h>

#include <QDomDocument>

using namespace foundation;

namespace deskew {
Params::Params(const double deskewAngleDeg, const Dependencies& deps, const AutoManualMode mode)
    : m_deskewAngleDeg(deskewAngleDeg), m_deps(deps), m_mode(mode) {}

Params::Params(const QDomElement& deskewEl)
    : m_deskewAngleDeg(deskewEl.attribute("angle").toDouble()),
      m_deps(deskewEl.namedItem("dependencies").toElement()),
      m_mode(deskewEl.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO) {}

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