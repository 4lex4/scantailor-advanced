// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Params.h"

#include <QDomDocument>

namespace page_split {
Params::Params(const PageLayout& layout, const Dependencies& deps, const AutoManualMode splitLineMode)
    : m_layout(layout), m_deps(deps), m_splitLineMode(splitLineMode) {}

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