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

#include "Dpi.h"
#include <Constants.h>
#include <QDomDocument>
#include <QDomElement>
#include "Dpm.h"

using namespace imageproc;

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
