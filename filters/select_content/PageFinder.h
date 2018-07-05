/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>
    Copyright (C) 2012  Petr Kovar <pejuko@gmail.com>

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

#ifndef SELECT_CONTENT_PAGEFINDER_H_
#define SELECT_CONTENT_PAGEFINDER_H_

#include "Margins.h"
#include "imageproc/BinaryThreshold.h"

#include <Qt>

class TaskStatus;
class DebugImages;
class FilterData;
class QImage;
class QRect;
class QRectF;
class QSizeF;
class QSize;

namespace imageproc {
class BinaryImage;
}

namespace select_content {
class PageFinder {
 public:
  static QRectF findPageBox(const TaskStatus& status,
                            const FilterData& data,
                            bool fine_tune,
                            const QSizeF& box,
                            double tolerance,
                            DebugImages* dbg = nullptr);

 private:
  static QRect detectBorders(const QImage& img);

  static int detectEdge(const QImage& img, int start, int end, int inc, int mid, Qt::Orientation orient);

  static void fineTuneCorners(const QImage& img, QRect& rect, const QSize& size, double tolerance);

  static bool fineTuneCorner(const QImage& img,
                             int& x,
                             int& y,
                             int max_x,
                             int max_y,
                             int inc_x,
                             int inc_y,
                             const QSize& size,
                             double tolerance);
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_PAGEFINDER_H_
