// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputFileParams.h"

#include <QDateTime>
#include <QDomDocument>
#include <QFileInfo>

namespace output {
OutputFileParams::OutputFileParams() : m_size(-1), m_modifiedTime(0) {}

OutputFileParams::OutputFileParams(const QFileInfo& fileInfo) : m_size(-1), m_modifiedTime(0) {
  if (fileInfo.exists()) {
    m_size = fileInfo.size();
    m_modifiedTime = fileInfo.lastModified().toSecsSinceEpoch();
  }
}

OutputFileParams::OutputFileParams(const QDomElement& el) : m_size(-1), m_modifiedTime(0) {
  if (el.hasAttribute("size")) {
    m_size = (qint64) el.attribute("size").toLongLong();
  }
  if (el.hasAttribute("mtime")) {
    m_modifiedTime = (time_t) el.attribute("mtime").toLongLong();
  }
}

QDomElement OutputFileParams::toXml(QDomDocument& doc, const QString& name) const {
  if (isValid()) {
    QDomElement el(doc.createElement(name));
    el.setAttribute("size", QString::number(m_size));
    el.setAttribute("mtime", QString::number(m_modifiedTime));
    return el;
  } else {
    return QDomElement();
  }
}
}  // namespace output
