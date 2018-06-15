/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DESPECKLE_H_
#define DESPECKLE_H_

class Dpi;
class TaskStatus;
class DebugImages;

namespace imageproc {
class BinaryImage;
}

class Despeckle {
 public:
  enum Level { CAUTIOUS, NORMAL, AGGRESSIVE };

  /**
   * \brief Removes small speckles from a binary image.
   *
   * \param src The image to despeckle.  Must not be null.
   * \param dpi DPI of \p src.
   * \param level Despeckling aggressiveness.
   * \param dbg An optional sink for debugging images.
   * \param status For asynchronous task cancellation.
   * \return The despeckled image.
   */
  static imageproc::BinaryImage despeckle(const imageproc::BinaryImage& src,
                                          const Dpi& dpi,
                                          Level level,
                                          const TaskStatus& status,
                                          DebugImages* dbg = nullptr);

  static imageproc::BinaryImage despeckle(const imageproc::BinaryImage& src,
                                          const Dpi& dpi,
                                          double level,
                                          const TaskStatus& status,
                                          DebugImages* dbg = nullptr);

  /**
   * \brief A slightly faster, in-place version of despeckle().
   */
  static void despeckleInPlace(imageproc::BinaryImage& image,
                               const Dpi& dpi,
                               Level level,
                               const TaskStatus& status,
                               DebugImages* dbg = nullptr);

  static void despeckleInPlace(imageproc::BinaryImage& image,
                               const Dpi& dpi,
                               double level,
                               const TaskStatus& status,
                               DebugImages* dbg = nullptr);
};


#endif
