// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkablePath.h"

#include <QStringList>

RelinkablePath::RelinkablePath(const QString& path, Type type) : m_normalizedPath(normalize(path)), m_type(type) {}

QString RelinkablePath::normalize(const QString& path) {
  QString frontSlashes(path);
  frontSlashes.replace(QChar('\\'), QLatin1String("/"));

  QStringList newComponents;
  for (const QString& comp : frontSlashes.split(QChar('/'), QString::KeepEmptyParts)) {
    if (comp.isEmpty()) {
      if (newComponents.isEmpty()
#if _WIN32
          || (newComponents.size() == 1 && newComponents.front().isEmpty())
#endif
      ) {
        newComponents.push_back(comp);
      } else {
        // This will get rid of redundant slashes, including the trailing slash.
        continue;
      }
    } else if (comp == ".") {
      continue;
    } else if (comp == "..") {
      if (newComponents.isEmpty()) {
        return QString();  // Error.
      }
      const QString& lastComp = newComponents.back();
      if (lastComp.isEmpty() || lastComp.endsWith(QChar(':'))) {
        return QString();  // Error.
      }
      newComponents.pop_back();
    } else {
      newComponents.push_back(comp);
    }
  }
  return newComponents.join(QChar('/'));
}  // RelinkablePath::normalize
