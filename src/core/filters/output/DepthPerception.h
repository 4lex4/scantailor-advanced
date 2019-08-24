// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_DEPTHPERCEPTION_H_
#define SCANTAILOR_OUTPUT_DEPTHPERCEPTION_H_

#include <QString>

namespace output {
/**
 * \see imageproc::CylindricalSurfaceDewarper
 */
class DepthPerception {
 public:
  DepthPerception();

  explicit DepthPerception(double value);

  explicit DepthPerception(const QString& fromString);

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
#endif  // ifndef SCANTAILOR_OUTPUT_DEPTHPERCEPTION_H_
