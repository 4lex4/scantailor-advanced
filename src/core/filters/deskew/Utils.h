// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_UTILS_H_
#define SCANTAILOR_DESKEW_UTILS_H_

namespace deskew {
class Params;

class Utils {
 public:
  Utils() = delete;

  static Params buildDefaultParams();
};
}  // namespace deskew

#endif  // SCANTAILOR_DESKEW_UTILS_H_
