/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>
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

#include "PageFinder.h"

#include "DebugImages.h"
#include "FilterData.h"
#include "TaskStatus.h"
#include "imageproc/Binarize.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/Transform.h"

#include <QDebug>

namespace select_content {
using namespace imageproc;

QRectF PageFinder::findPageBox(const TaskStatus& status,
                               const FilterData& data,
                               bool fine_tune,
                               const QSizeF& box,
                               double tolerance,
                               DebugImages* dbg) {
  ImageTransformation xform_150dpi(data.xform());
  xform_150dpi.preScaleToDpi(Dpi(150, 150));

  if (xform_150dpi.resultingRect().toRect().isEmpty()) {
    return QRectF();
  }

  double to150 = 150.0 / 25.4;
  auto exp_width = int(to150 * box.width());
  auto exp_height = int(to150 * box.height());

#ifdef DEBUG
  std::cout << "dpi: " << data.xform().origDpi().horizontal() << std::endl;
  std::cout << "tolerance: " << tolerance << std::endl;
  std::cout << "exp_width = " << exp_width << "; exp_height" << exp_height << std::endl;
#endif

  const GrayImage dataGrayImage = data.isBlackOnWhite() ? data.grayImage() : data.grayImage().inverted();
  const uint8_t darkest_gray_level = darkestGrayLevel(dataGrayImage);
  const QColor outside_color(darkest_gray_level, darkest_gray_level, darkest_gray_level);

  QImage gray150(transformToGray(dataGrayImage, xform_150dpi.transform(), xform_150dpi.resultingRect().toRect(),
                                 OutsidePixels::assumeColor(outside_color)));
  if (dbg) {
    dbg->add(gray150, "gray150");
  }


  std::vector<QRect> rects;
  std::vector<BinaryImage> bwimages;


  bwimages.push_back(peakThreshold(gray150));
  bwimages.push_back(binarizeOtsu(gray150));
  bwimages.push_back(binarizeMokji(gray150));
  bwimages.push_back(binarizeSauvola(gray150, gray150.size()));
  bwimages.push_back(binarizeWolf(gray150, gray150.size()));
  if (dbg) {
    dbg->add(bwimages[0], "peakThreshold");
    dbg->add(bwimages[1], "OtsuThreshold");
    dbg->add(bwimages[2], "MokjiThreshold");
    dbg->add(bwimages[3], "SauvolaThreshold");
    dbg->add(bwimages[4], "WolfThreshold");
  }

  QRect content_rect(0, 0, 0, 0);
  double err_width = 1.0;
  double err_height = 1.0;

  if (box.isEmpty()) {
    QImage bwimg(bwimages[3].toQImage());
    content_rect = detectBorders(bwimg);
    if (fine_tune) {
      fineTuneCorners(bwimg, content_rect, QSize(0, 0), 1.0);
    }
  } else {
    for (int i = 0; i < bwimages.size(); ++i) {
      QImage bwimg(bwimages[i].toQImage());
      rects.push_back(QRect(detectBorders(bwimg)));
      if (fine_tune) {
        fineTuneCorners(bwimg, rects[i], QSize(exp_width, exp_height), tolerance);
      }
#ifdef DEBUG
      std::cout << "width = " << rects[i].width() << "; height=" << rects[i].height() << std::endl;
#endif

      double err_w = double(std::abs(exp_width - rects[i].width())) / double(exp_width);
      double err_h = double(std::abs(exp_height - rects[i].height())) / double(exp_height);
#ifdef DEBUG
      std::cout << "err_w=" << err_w << "; err_h" << err_h << std::endl;
#endif

      if (err_w < err_width) {
        content_rect.setLeft(rects[i].left());
        content_rect.setRight(rects[i].right());
        err_width = err_w;
      }
      if (err_h < err_height) {
        content_rect.setTop(rects[i].top());
        content_rect.setBottom(rects[i].bottom());
        err_height = err_h;
      }
    }
  }
#ifdef DEBUG
  std::cout << "width = " << content_rect.width() << "; height=" << content_rect.height() << std::endl;
#endif

  QTransform combined_xform(xform_150dpi.transform().inverted());
  combined_xform *= data.xform().transform();
  QRectF result = combined_xform.map(QRectF(content_rect)).boundingRect();

  return result;
}  // PageFinder::findPageBox

QRect PageFinder::detectBorders(const QImage& img) {
  int l = 0, t = 0, r = img.width() - 1, b = img.height() - 1;
  int xmid = r / 2;
  int ymid = b / 2;

  l = detectEdge(img, l, r, 1, ymid, Qt::Horizontal);
  t = detectEdge(img, t, b, 1, xmid, Qt::Vertical);
  r = detectEdge(img, r, 0, -1, ymid, Qt::Horizontal);
  b = detectEdge(img, b, t, -1, xmid, Qt::Vertical);

  return QRect(l, t, r - l + 1, b - t + 1);
}

/**
 * shift edge while points around mid are black
 */
int PageFinder::detectEdge(const QImage& img, int start, int end, int inc, int mid, Qt::Orientation orient) {
  int min_size = 10;
  int gap = 0;
  int i = start, edge = start;
  int ms = 0;
  int me = 2 * mid;
  auto min_bp = int(double(me - ms) * 0.95);
  Qt::GlobalColor black = Qt::color1;

  while (i != end) {
    int black_pixels = 0;
    int old_gap = gap;

    for (int j = ms; j != me; j++) {
      int x = i, y = j;
      if (orient == Qt::Vertical) {
        x = j;
        y = i;
      }
      int pixel = img.pixelIndex(x, y);
      if (pixel == black) {
        ++black_pixels;
      }
    }

    if (black_pixels < min_bp) {
      ++gap;
    } else {
      gap = 0;
      edge = i;
    }

    if (gap > min_size) {
      break;
    }

    i += inc;
  }

  return edge;
}  // PageFinder::detectEdge

void PageFinder::fineTuneCorners(const QImage& img, QRect& rect, const QSize& size, double tolerance) {
  int l = rect.left(), t = rect.top(), r = rect.right(), b = rect.bottom();
  bool done = false;

  while (!done) {
    done = fineTuneCorner(img, l, t, r, b, 1, 1, size, tolerance);
    done &= fineTuneCorner(img, r, t, l, b, -1, 1, size, tolerance);
    done &= fineTuneCorner(img, l, b, r, t, 1, -1, size, tolerance);
    done &= fineTuneCorner(img, r, b, l, t, -1, -1, size, tolerance);
  }

  rect.setLeft(l);
  rect.setTop(t);
  rect.setRight(r);
  rect.setBottom(b);
}

/**
 * shift edges until given corner is out of black
 */
bool PageFinder::fineTuneCorner(const QImage& img,
                                int& x,
                                int& y,
                                int max_x,
                                int max_y,
                                int inc_x,
                                int inc_y,
                                const QSize& size,
                                double tolerance) {
  auto width_t = static_cast<int>(size.width() * (1.0 - tolerance));
  auto height_t = static_cast<int>(size.height() * (1.0 - tolerance));

  Qt::GlobalColor black = Qt::color1;
  int pixel = img.pixelIndex(x, y);
  int tx = x + inc_x;
  int ty = y + inc_y;
  int w = std::abs(max_x - x);
  int h = std::abs(max_y - y);

  if ((!size.isEmpty()) && ((w < width_t) || (h < height_t))) {
    return true;
  }
  if ((pixel != black) || (tx < 0) || (tx > (img.width() - 1)) || (ty < 0) || (ty > (img.height() - 1))) {
    return true;
  }
  x = tx;
  y = ty;

  return false;
}
}  // namespace select_content