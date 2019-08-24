// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RecentProjects.h"
#include <QFile>
#include <QSettings>

void RecentProjects::read() {
  QSettings settings;
  std::list<QString> newList;

  const int size = settings.beginReadArray("project/recent");
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    const QString path(settings.value("path").toString());
    newList.push_back(path);
  }
  settings.endArray();

  m_projectFiles.swap(newList);
}

void RecentProjects::write(const int maxItems) const {
  QSettings settings;
  settings.beginWriteArray("project/recent");
  int idx = 0;
  for (const QString& path : m_projectFiles) {
    if (idx >= maxItems) {
      break;
    }
    settings.setArrayIndex(idx);
    settings.setValue("path", path);
    ++idx;
  }
  settings.endArray();
}

bool RecentProjects::validate() {
  bool allOk = true;

  auto it(m_projectFiles.begin());
  const auto end(m_projectFiles.end());
  while (it != end) {
    if (QFile::exists(*it)) {
      ++it;
    } else {
      m_projectFiles.erase(it++);
      allOk = false;
    }
  }

  return allOk;
}

void RecentProjects::setMostRecent(const QString& filePath) {
  const auto begin(m_projectFiles.begin());
  const auto end(m_projectFiles.end());
  auto it(std::find(begin, end, filePath));
  if (it != end) {
    m_projectFiles.splice(begin, m_projectFiles, it);
  } else {
    m_projectFiles.push_front(filePath);
  }
}
