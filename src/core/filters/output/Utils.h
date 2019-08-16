// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_UTILS_H_
#define OUTPUT_UTILS_H_

class Dpi;
class QString;
class QTransform;
class QRect;

namespace output {
class Utils {
 public:
  static QString automaskDir(const QString& out_dir);

  static QString predespeckleDir(const QString& out_dir);

  static QString specklesDir(const QString& out_dir);

  static QString foregroundDir(const QString& out_dir);

  static QString backgroundDir(const QString& out_dir);

  static QString originalBackgroundDir(const QString& out_dir);

  static QTransform scaleFromToDpi(const Dpi& from, const Dpi& to);

  static QTransform rotate(double degrees, const QRect& image_rect);

  Utils() = delete;
};
}  // namespace output
#endif
