// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_UNITSCONVERTER_H_
#define SCANTAILOR_CORE_UNITSCONVERTER_H_


#include <QtGui/QTransform>

#include "Dpi.h"
#include "Units.h"

class UnitsConverter {
 public:
  UnitsConverter() = default;

  explicit UnitsConverter(const Dpi& dpi);

  void convert(double& horizontalValue, double& verticalValue, Units fromUnits, Units toUnits) const;

  QTransform transform(Units fromUnits, Units toUnits) const;

  const Dpi& getDpi() const { return m_dpi; }

  void setDpi(const Dpi& dpi) { m_dpi = dpi; }

 private:
  Dpi m_dpi;
};


#endif  // SCANTAILOR_CORE_UNITSCONVERTER_H_
