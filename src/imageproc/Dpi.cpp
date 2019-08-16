// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Dpi.h"
#include <Constants.h>
#include <QDomDocument>
#include <QDomElement>
#include "Dpm.h"

Dpi::Dpi() : m_xDpi(0), m_yDpi(0) {}

Dpi::Dpi(int horizontal, int vertical) : m_xDpi(horizontal), m_yDpi(vertical) {}

Dpi::Dpi(const QSize size) : m_xDpi(size.width()), m_yDpi(size.height()) {}

Dpi::Dpi(const Dpm dpm)
    : m_xDpi(qRound(dpm.horizontal() * constants::DPM2DPI)), m_yDpi(qRound(dpm.vertical() * constants::DPM2DPI)) {}

Dpi::Dpi(const QDomElement& el)
    : m_xDpi(el.attribute("horizontal").toInt()), m_yDpi(el.attribute("vertical").toInt()) {}

QSize Dpi::toSize() const {
  if (isNull()) {
    return QSize();
  } else {
    return QSize(m_xDpi, m_yDpi);
  }
}

bool Dpi::operator==(const Dpi& other) const {
  return m_xDpi == other.m_xDpi && m_yDpi == other.m_yDpi;
}

bool Dpi::operator!=(const Dpi& other) const {
  return !(*this == other);
}

QDomElement Dpi::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("horizontal", m_xDpi);
  el.setAttribute("vertical", m_yDpi);
  return el;
}

int Dpi::horizontal() const {
  return m_xDpi;
}

int Dpi::vertical() const {
  return m_yDpi;
}

bool Dpi::isNull() const {
  return m_xDpi <= 1 || m_yDpi <= 1;
}
