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

namespace page_split {
Params::Params(const PageLayout& layout, const Dependencies& deps, const AutoManualMode split_line_mode)
    : m_layout(layout), m_deps(deps), m_splitLineMode(split_line_mode) {}

Params::Params(const QDomElement& el)
    : m_layout(el.namedItem("pages").toElement()),
      m_deps(el.namedItem("dependencies").toElement()),
      m_splitLineMode(el.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO) {}

Params::~Params() = default;

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("mode", m_splitLineMode == MODE_AUTO ? "auto" : "manual");
  el.appendChild(m_layout.toXml(doc, "pages"));
  el.appendChild(m_deps.toXml(doc, "dependencies"));

  return el;
}

const PageLayout& Params::pageLayout() const {
  return m_layout;
}

void Params::setPageLayout(const PageLayout& layout) {
  m_layout = layout;
}

const Dependencies& Params::dependencies() const {
  return m_deps;
}

void Params::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

AutoManualMode Params::splitLineMode() const {
  return m_splitLineMode;
}

void Params::setSplitLineMode(AutoManualMode mode) {
  m_splitLineMode = mode;
}
}  // namespace page_split