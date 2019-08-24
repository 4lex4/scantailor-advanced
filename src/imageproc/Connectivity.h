// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_CONNECTIVITY_H_
#define SCANTAILOR_IMAGEPROC_CONNECTIVITY_H_

namespace imageproc {
/**
 * \brief Defines which neighbouring pixels are considered to be connected.
 */
enum Connectivity {
  /** North, east, south and west neighbours of a pixel
      are considered to be connected to it. */
  CONN4,

  /** All 8 neighbours are considered to be connected. */
  CONN8
};
}  // namespace imageproc
#endif
