// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_DEPTH_PERCEPTION_H_
#define OUTPUT_DEPTH_PERCEPTION_H_

#include <QString>

namespace output {
/**
 * \see imageproc::CylindricalSurfaceDewarper
 */
class DepthPerception {
 public:
  DepthPerception();

  explicit DepthPerception(double value);

  explicit DepthPerception(const QString& from_string);

  QString toString() const;

  void setValue(double value);

  double value() const;

  static double minValue();

  static double defaultValue();

  static double maxValue();

 private:
  double m_value;
};
}  // namespace output
#endif  // ifndef OUTPUT_DEPTH_PERCEPTION_H_
