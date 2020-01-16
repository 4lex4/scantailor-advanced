// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Margins.h"

#include <foundation/Utils.h>

#include <QDomDocument>
#include <QDomElement>
#include <QString>

using namespace foundation;

Margins::Margins() : m_top(), m_bottom(), m_left(), m_right() {}

Margins::Margins(double left, double top, double right, double bottom)
    : m_top(top), m_bottom(bottom), m_left(left), m_right(right) {}

Margins::Margins(const QDomElement& el)
    : m_top(el.attribute("top").toDouble()),
      m_bottom(el.attribute("bottom").toDouble()),
      m_left(el.attribute("left").toDouble()),
      m_right(el.attribute("right").toDouble()) {}

QDomElement Margins::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("left", Utils::doubleToString(m_left));
  el.setAttribute("right", Utils::doubleToString(m_right));
  el.setAttribute("top", Utils::doubleToString(m_top));
  el.setAttribute("bottom", Utils::doubleToString(m_bottom));
  return el;
}

double Margins::top() const {
  return m_top;
}

void Margins::setTop(double val) {
  m_top = val;
}

double Margins::bottom() const {
  return m_bottom;
}

void Margins::setBottom(double val) {
  m_bottom = val;
}

double Margins::left() const {
  return m_left;
}

void Margins::setLeft(double val) {
  m_left = val;
}

double Margins::right() const {
  return m_right;
}

void Margins::setRight(double val) {
  m_right = val;
}
