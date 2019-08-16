// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DESKEW_PARAMS_H_
#define DESKEW_PARAMS_H_

#include <QString>
#include <algorithm>
#include <cmath>
#include "AutoManualMode.h"
#include "Dependencies.h"

class QDomDocument;
class QDomElement;

namespace deskew {
class Params {
 public:
  // Member-wise copying is OK.

  Params(double deskew_angle_deg, const Dependencies& deps, AutoManualMode mode);

  explicit Params(const QDomElement& deskew_el);

  ~Params();

  double deskewAngle() const;

  const Dependencies& dependencies() const;

  AutoManualMode mode() const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  double m_deskewAngleDeg;
  Dependencies m_deps;
  AutoManualMode m_mode;
};
}  // namespace deskew
#endif  // ifndef DESKEW_PARAMS_H_
