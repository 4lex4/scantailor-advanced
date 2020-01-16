// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Dependencies.h"

#include <PolygonUtils.h>

#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

using namespace imageproc;

namespace deskew {
Dependencies::Dependencies() = default;

Dependencies::Dependencies(const QPolygonF& pageOutline, const OrthogonalRotation rotation)
    : m_pageOutline(pageOutline), m_rotation(rotation) {}

Dependencies::Dependencies(const QDomElement& depsEl)
    : m_pageOutline(XmlUnmarshaller::polygonF(depsEl.namedItem("page-outline").toElement())),
      m_rotation(depsEl.namedItem("rotation").toElement()) {}

Dependencies::~Dependencies() = default;

bool Dependencies::matches(const Dependencies& other) const {
  if (m_rotation != other.m_rotation) {
    return false;
  }
  return PolygonUtils::fuzzyCompare(m_pageOutline, other.m_pageOutline);
}

QDomElement Dependencies::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(m_rotation.toXml(doc, "rotation"));
  el.appendChild(marshaller.polygonF(m_pageOutline, "page-outline"));
  return el;
}
}  // namespace deskew