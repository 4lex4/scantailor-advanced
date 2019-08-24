// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_BWCOLOR_H_
#define SCANTAILOR_IMAGEPROC_BWCOLOR_H_

namespace imageproc {
enum BWColor { WHITE = 0, BLACK = 1 };

inline BWColor operator!(BWColor c) {
  return static_cast<BWColor>(~c & 1);
}
}  // namespace imageproc
#endif
