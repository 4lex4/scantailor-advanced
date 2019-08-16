// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SmartFilenameOrdering.h"
#include <QFileInfo>

bool SmartFilenameOrdering::operator()(const QFileInfo& lhs, const QFileInfo& rhs) const {
  // First compare directories.
  if (int comp = lhs.absolutePath().compare(rhs.absolutePath())) {
    return comp < 0;
  }

  const QString lhs_fname(lhs.fileName());
  const QString rhs_fname(rhs.fileName());
  const QChar* lhs_ptr = lhs_fname.constData();
  const QChar* rhs_ptr = rhs_fname.constData();
  while (!lhs_ptr->isNull() && !rhs_ptr->isNull()) {
    const bool lhs_is_digit = lhs_ptr->isDigit();
    const bool rhs_is_digit = rhs_ptr->isDigit();
    if (lhs_is_digit != rhs_is_digit) {
      // Digits have priority over non-digits.
      return lhs_is_digit;
    }

    if (lhs_is_digit && rhs_is_digit) {
      unsigned long lhs_number = 0;
      do {
        lhs_number = lhs_number * 10 + lhs_ptr->digitValue();
        ++lhs_ptr;
        // Note: isDigit() implies !isNull()
      } while (lhs_ptr->isDigit());

      unsigned long rhs_number = 0;
      do {
        rhs_number = rhs_number * 10 + rhs_ptr->digitValue();
        ++rhs_ptr;
        // Note: isDigit() implies !isNull()
      } while (rhs_ptr->isDigit());

      if (lhs_number != rhs_number) {
        return lhs_number < rhs_number;
      } else {
        continue;
      }
    }

    if (lhs_ptr->isNull() != rhs_ptr->isNull()) {
      return *lhs_ptr < *rhs_ptr;
    }

    ++lhs_ptr;
    ++rhs_ptr;
  }

  if (!lhs_ptr->isNull() || !rhs_ptr->isNull()) {
    return lhs_ptr->isNull();
  }

  // OK, the smart comparison indicates the file names are equal.
  // However, if they aren't symbol-to-symbol equal, we can't treat
  // them as equal, so let's do a usual comparision now.
  return lhs_fname < rhs_fname;
}  // ()
