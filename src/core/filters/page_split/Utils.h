// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_UTILS_H_
#define SCANTAILOR_PAGE_SPLIT_UTILS_H_

#include "Settings.h"

namespace page_split {
class Utils {
 public:
  Utils() = delete;

  static Settings::UpdateAction buildDefaultUpdateAction();
};
}  // namespace page_split

#endif  // SCANTAILOR_PAGE_SPLIT_UTILS_H_
