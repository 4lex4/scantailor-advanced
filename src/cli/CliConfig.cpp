// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CliConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

bool CliConfig::loadFromFile(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }
  const QString configDir = QFileInfo(path).absolutePath();
  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
      continue;
    }
    int eq = line.indexOf(QLatin1Char('='));
    if (eq <= 0) {
      continue;
    }
    QString key = line.left(eq).trimmed();
    QString value = line.mid(eq + 1).trimmed();
    if (key == QLatin1String("project")) {
      if (!value.isEmpty() && !QDir::isAbsolutePath(value)) {
        projectPath = QDir(configDir).absoluteFilePath(value);
      } else {
        projectPath = value;
      }
    } else if (key == QLatin1String("threads")) {
      bool ok = false;
      int n = value.toInt(&ok);
      if (ok && n > 0) {
        threads = n;
      }
    } else if (key == QLatin1String("output-dir")) {
      outputDir = value;
    }
  }
  return true;
}
