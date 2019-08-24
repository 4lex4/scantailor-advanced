// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_UTILS_H_
#define SCANTAILOR_OUTPUT_UTILS_H_

class Dpi;
class QString;
class QTransform;
class QRect;

namespace output {
class Utils {
 public:
  static QString automaskDir(const QString& outDir);

  static QString predespeckleDir(const QString& outDir);

  static QString specklesDir(const QString& outDir);

  static QString foregroundDir(const QString& outDir);

  static QString backgroundDir(const QString& outDir);

  static QString originalBackgroundDir(const QString& outDir);

  static QTransform scaleFromToDpi(const Dpi& from, const Dpi& to);

  static QTransform rotate(double degrees, const QRect& imageRect);

  Utils() = delete;
};
}  // namespace output
#endif
