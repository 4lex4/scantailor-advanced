// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_UTILS_H
#define SCANTAILOR_FOUNDATION_UTILS_H

#include <QString>

namespace foundation {
class Utils {
 public:
  /**
   * \brief A high precision, locale independent number to string conversion.
   *
   * This function is intended to be used instead of
   * QDomElement::setAttribute(double), which is locale dependent.
   */
  static QString doubleToString(double val);

  Utils() = delete;
};
}


#endif //SCANTAILOR_UTILS_H
