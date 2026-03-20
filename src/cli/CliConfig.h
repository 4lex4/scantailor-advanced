// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_CLICONFIG_H_
#define SCANTAILOR_CLI_CLICONFIG_H_

#include <QString>

struct CliConfig {
  QString projectPath;
  int threads = -1;  // -1 = use default
  QString outputDir;  // empty = use project's

  bool loadFromFile(const QString& path);
};

#endif  // SCANTAILOR_CLI_CLICONFIG_H_
