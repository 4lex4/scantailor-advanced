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

  static constexpr double minValue();

  static constexpr double defaultValue();

  static constexpr double maxValue();

 private:
  double m_value;
};

inline double DepthPerception::value() const {
  return m_value;
}

constexpr double DepthPerception::minValue() {
  return 1.0;
}

constexpr double DepthPerception::defaultValue() {
  return 2.0;
}

constexpr double DepthPerception::maxValue() {
  return 3.0;
}
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_DEPTHPERCEPTION_H_
