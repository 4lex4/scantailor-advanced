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

#include "Alignment.h"
#include <QDomDocument>

namespace page_layout {
Alignment::Alignment() : m_vertical(VCENTER), m_horizontal(HCENTER), m_isNull(false) {}

Alignment::Alignment(Vertical vertical, Horizontal horizontal)
    : m_vertical(vertical), m_horizontal(horizontal), m_isNull(false) {}

Alignment::Alignment(const QDomElement& el) {
  const QString vert(el.attribute("vert"));
  const QString hor(el.attribute("hor"));
  m_isNull = el.attribute("null").toInt() != 0;

  if (vert == "top") {
    m_vertical = TOP;
  } else if (vert == "bottom") {
    m_vertical = BOTTOM;
  } else if (vert == "auto") {
    m_vertical = VAUTO;
  } else if (vert == "original") {
    m_vertical = VORIGINAL;
  } else {
    m_vertical = VCENTER;
  }

  if (hor == "left") {
    m_horizontal = LEFT;
  } else if (hor == "right") {
    m_horizontal = RIGHT;
  } else if (hor == "auto") {
    m_horizontal = HAUTO;
  } else if (vert == "original") {
    m_horizontal = HORIGINAL;
  } else {
    m_horizontal = HCENTER;
  }
}

QDomElement Alignment::toXml(QDomDocument& doc, const QString& name) const {
  const char* vert = nullptr;
  switch (m_vertical) {
    case TOP:
      vert = "top";
      break;
    case VCENTER:
      vert = "vcenter";
      break;
    case BOTTOM:
      vert = "bottom";
      break;
    case VAUTO:
      vert = "auto";
      break;
    case VORIGINAL:
      vert = "original";
      break;
  }

  const char* hor = nullptr;
  switch (m_horizontal) {
    case LEFT:
      hor = "left";
      break;
    case HCENTER:
      hor = "hcenter";
      break;
    case RIGHT:
      hor = "right";
      break;
    case HAUTO:
      hor = "auto";
      break;
    case HORIGINAL:
      hor = "original";
      break;
  }

  QDomElement el(doc.createElement(name));
  el.setAttribute("vert", QString::fromLatin1(vert));
  el.setAttribute("hor", QString::fromLatin1(hor));
  el.setAttribute("null", m_isNull ? 1 : 0);

  return el;
}

bool Alignment::operator==(const Alignment& other) const {
  return (m_vertical == other.m_vertical) && (m_horizontal == other.m_horizontal) && (m_isNull == other.m_isNull);
}

bool Alignment::operator!=(const Alignment& other) const {
  return !(*this == other);
}

Alignment::Vertical Alignment::vertical() const {
  return m_vertical;
}

void Alignment::setVertical(Alignment::Vertical vertical) {
  m_vertical = vertical;
}

Alignment::Horizontal Alignment::horizontal() const {
  return m_horizontal;
}

void Alignment::setHorizontal(Alignment::Horizontal horizontal) {
  m_horizontal = horizontal;
}

bool Alignment::isNull() const {
  return m_isNull;
}

void Alignment::setNull(bool is_null) {
  m_isNull = is_null;
}

bool Alignment::isAutoVertical() const {
  return (m_vertical == VAUTO) || (m_vertical == VORIGINAL);
}

bool Alignment::isAutoHorizontal() const {
  return (m_horizontal == HAUTO) || (m_horizontal == HORIGINAL);
}

bool Alignment::isOriginal() const {
  return (m_vertical == VORIGINAL) || (m_horizontal == HORIGINAL);
}

bool Alignment::isAuto() const {
  return (m_vertical == VAUTO) || (m_horizontal == HAUTO);
}
}  // namespace page_layout
