/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
