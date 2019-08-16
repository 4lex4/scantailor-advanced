// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
