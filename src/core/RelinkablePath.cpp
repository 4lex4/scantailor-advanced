// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkablePath.h"
#include <QStringList>

RelinkablePath::RelinkablePath(const QString& path, Type type) : m_normalizedPath(normalize(path)), m_type(type) {}

QString RelinkablePath::normalize(const QString& path) {
  QString front_slashes(path);
  front_slashes.replace(QChar('\\'), QLatin1String("/"));

  QStringList new_components;
  for (const QString& comp : front_slashes.split(QChar('/'), QString::KeepEmptyParts)) {
    if (comp.isEmpty()) {
      if (new_components.isEmpty()
#if _WIN32
          || (new_components.size() == 1 && new_components.front().isEmpty())
#endif
      ) {
        new_components.push_back(comp);
      } else {
        // This will get rid of redundant slashes, including the trailing slash.
        continue;
      }
    } else if (comp == ".") {
      continue;
    } else if (comp == "..") {
      if (new_components.isEmpty()) {
        return QString();  // Error.
      }
      const QString& last_comp = new_components.back();
      if (last_comp.isEmpty() || last_comp.endsWith(QChar(':'))) {
        return QString();  // Error.
      }
      new_components.pop_back();
    } else {
      new_components.push_back(comp);
    }
  }

  return new_components.join(QChar('/'));
}  // RelinkablePath::normalize
