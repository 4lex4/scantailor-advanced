// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LayoutType.h"
#include <cassert>

namespace page_split {
QString layoutTypeToString(const LayoutType layout_type) {
  switch (layout_type) {
    case AUTO_LAYOUT_TYPE:
      return "auto-detect";
    case SINGLE_PAGE_UNCUT:
      return "single-uncut";
    case PAGE_PLUS_OFFCUT:
      return "single-cut";
    case TWO_PAGES:
      return "two-pages";
  }
  assert(!"unreachable");

  return QString();
}

LayoutType layoutTypeFromString(const QString& layout_type) {
  if (layout_type == "single-uncut") {
    return SINGLE_PAGE_UNCUT;
  } else if (layout_type == "single-cut") {
    return PAGE_PLUS_OFFCUT;
  } else if (layout_type == "two-pages") {
    return TWO_PAGES;
  } else {
    return AUTO_LAYOUT_TYPE;
  }
}
}  // namespace page_split