// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_PROJECTGENERATORFROMDIR_H_
#define SCANTAILOR_CLI_PROJECTGENERATORFROMDIR_H_

#include <QString>

class ProjectGeneratorFromDir {
 public:
  /**
   * Genera project.ScanTailor en \p imagesDir con todas las imágenes encontradas (todo en automático).
   * \p outputDirName es el nombre del subdirectorio de salida (ej. "out").
   * Devuelve la ruta absoluta del proyecto creado, o vacía si falla.
   */
  static QString generate(const QString& imagesDir, const QString& outputDirName = QStringLiteral("out"));
};

#endif  // SCANTAILOR_CLI_PROJECTGENERATORFROMDIR_H_
