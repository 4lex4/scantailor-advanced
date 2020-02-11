// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_UTILS_H_
#define SCANTAILOR_FIX_ORIENTATION_UTILS_H_

class OrthogonalRotation;

namespace fix_orientation {
class Utils {
 public:
  Utils() = delete;

  static OrthogonalRotation getDefaultOrthogonalRotation();
};
}  // namespace fix_orientation

#endif  // SCANTAILOR_FIX_ORIENTATION_UTILS_H_
