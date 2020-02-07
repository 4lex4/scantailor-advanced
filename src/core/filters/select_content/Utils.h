// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_UTILS_H_
#define SCANTAILOR_SELECT_CONTENT_UTILS_H_

class Dpi;

namespace select_content {
class Params;

class Utils {
 public:
  Utils() = delete;

  static Params buildDefaultParams(const Dpi& dpi);
};
}  // namespace select_content

#endif  // SCANTAILOR_SELECT_CONTENT_UTILS_H_
