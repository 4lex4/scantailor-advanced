// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECT_CONTENT_PAGEFINDER_H_
#define SELECT_CONTENT_PAGEFINDER_H_

#include "Margins.h"
#include <BinaryThreshold.h>

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
