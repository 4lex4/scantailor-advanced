// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SmartFilenameOrdering.h"

#include <QFileInfo>
#include <QString>

bool SmartFilenameOrdering::operator()(const QFileInfo& lhs, const QFileInfo& rhs) const {
  // First compare directories.
  if (int comp = lhs.absolutePath().compare(rhs.absolutePath())) {
    return comp < 0;
  }

  const QString lhsFname(lhs.fileName());
  const QString rhsFname(rhs.fileName());
  const QChar* lhsPtr = lhsFname.constData();
  const QChar* rhsPtr = rhsFname.constData();
  while (!lhsPtr->isNull() && !rhsPtr->isNull()) {
    const bool lhsIsDigit = lhsPtr->isDigit();
    const bool rhsIsDigit = rhsPtr->isDigit();
    if (lhsIsDigit != rhsIsDigit) {
      // Digits have priority over non-digits.
      return lhsIsDigit;
    }

    if (lhsIsDigit && rhsIsDigit) {
      unsigned long lhsNumber = 0;
      do {
        lhsNumber = lhsNumber * 10 + lhsPtr->digitValue();
        ++lhsPtr;
        // Note: isDigit() implies !isNull()
      } while (lhsPtr->isDigit());

      unsigned long rhsNumber = 0;
      do {
        rhsNumber = rhsNumber * 10 + rhsPtr->digitValue();
        ++rhsPtr;
        // Note: isDigit() implies !isNull()
      } while (rhsPtr->isDigit());

      if (lhsNumber != rhsNumber) {
        return lhsNumber < rhsNumber;
      } else {
        continue;
      }
    }

    if (lhsPtr->isNull() != rhsPtr->isNull()) {
      return *lhsPtr < *rhsPtr;
    }

    ++lhsPtr;
    ++rhsPtr;
  }

  if (!lhsPtr->isNull() || !rhsPtr->isNull()) {
    return lhsPtr->isNull();
  }

  // OK, the smart comparison indicates the file names are equal.
  // However, if they aren't symbol-to-symbol equal, we can't treat
  // them as equal, so let's do a usual comparision now.
  return lhsFname < rhsFname;
}  // ()

bool SmartFilenameOrdering::operator()(QString const& lhs, QString const& rhs) const
{
        QFileInfo lhs_i(lhs);
        QFileInfo rhs_i(rhs);
        return this->operator()(lhs_i, rhs_i);
}

