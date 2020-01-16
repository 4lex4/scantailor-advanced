// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageFinder.h"

#include <Binarize.h>
#include <BinaryImage.h>
#include <GrayRasterOp.h>
#include <Transform.h>

#include <QDebug>

#include "DebugImages.h"
#include "FilterData.h"
#include "TaskStatus.h"

namespace select_content {
using namespace imageproc;

QRectF PageFinder::findPageBox(const TaskStatus& status,
                               const FilterData& data,
                               bool fineTune,
                               const QSizeF& box,
                               double tolerance,
                               DebugImages* dbg) {
  ImageTransformation xform150dpi(data.xform());
  xform150dpi.preScaleToDpi(Dpi(150, 150));

  if (xform150dpi.resultingRect().toRect().isEmpty()) {
    return QRectF();
  }

  double to150 = 150.0 / 25.4;
  auto expWidth = int(to150 * box.width());
  auto expHeight = int(to150 * box.height());

#ifdef DEBUG
  std::cout << "dpi: " << data.xform().origDpi().horizontal() << std::endl;
  std::cout << "tolerance: " << tolerance << std::endl;
  std::cout << "expWidth = " << expWidth << "; expHeight" << expHeight << std::endl;
#endif

  const GrayImage dataGrayImage = data.grayImageBlackOnWhite();
  const uint8_t darkestGrayLevel = imageproc::darkestGrayLevel(dataGrayImage);
  const QColor outsideColor(darkestGrayLevel, darkestGrayLevel, darkestGrayLevel);

  QImage gray150(transformToGray(dataGrayImage, xform150dpi.transform(), xform150dpi.resultingRect().toRect(),
                                 OutsidePixels::assumeColor(outsideColor)));
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

  QRect contentRect(0, 0, 0, 0);
  double errWidth = 1.0;
  double errHeight = 1.0;

  if (box.isEmpty()) {
    QImage bwimg(bwimages[3].toQImage());
    contentRect = detectBorders(bwimg);
    if (fineTune) {
      fineTuneCorners(bwimg, contentRect, QSize(0, 0), 1.0);
    }
  } else {
    for (uint32_t i = 0; i < bwimages.size(); ++i) {
      QImage bwimg(bwimages[i].toQImage());
      rects.push_back(QRect(detectBorders(bwimg)));
      if (fineTune) {
        fineTuneCorners(bwimg, rects[i], QSize(expWidth, expHeight), tolerance);
      }
#ifdef DEBUG
      std::cout << "width = " << rects[i].width() << "; height=" << rects[i].height() << std::endl;
#endif

      double errW = double(std::abs(expWidth - rects[i].width())) / double(expWidth);
      double errH = double(std::abs(expHeight - rects[i].height())) / double(expHeight);
#ifdef DEBUG
      std::cout << "errW=" << errW << "; errH" << errH << std::endl;
#endif

      if (errW < errWidth) {
        contentRect.setLeft(rects[i].left());
        contentRect.setRight(rects[i].right());
        errWidth = errW;
      }
      if (errH < errHeight) {
        contentRect.setTop(rects[i].top());
        contentRect.setBottom(rects[i].bottom());
        errHeight = errH;
      }
    }
  }
#ifdef DEBUG
  std::cout << "width = " << contentRect.width() << "; height=" << contentRect.height() << std::endl;
#endif

  QTransform combinedXform(xform150dpi.transform().inverted());
  combinedXform *= data.xform().transform();
  QRectF result = combinedXform.map(QRectF(contentRect)).boundingRect();
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
  int minSize = 10;
  int gap = 0;
  int i = start, edge = start;
  int ms = 0;
  int me = 2 * mid;
  auto minBp = int(double(me - ms) * 0.95);
  Qt::GlobalColor black = Qt::color1;

  while (i != end) {
    int blackPixels = 0;

    for (int j = ms; j != me; j++) {
      int x = i, y = j;
      if (orient == Qt::Vertical) {
        x = j;
        y = i;
      }
      int pixel = img.pixelIndex(x, y);
      if (pixel == black) {
        ++blackPixels;
      }
    }

    if (blackPixels < minBp) {
      ++gap;
    } else {
      gap = 0;
      edge = i;
    }

    if (gap > minSize) {
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
                                int maxX,
                                int maxY,
                                int incX,
                                int incY,
                                const QSize& size,
                                double tolerance) {
  auto widthT = static_cast<int>(size.width() * (1.0 - tolerance));
  auto heightT = static_cast<int>(size.height() * (1.0 - tolerance));

  Qt::GlobalColor black = Qt::color1;
  int pixel = img.pixelIndex(x, y);
  int tx = x + incX;
  int ty = y + incY;
  int w = std::abs(maxX - x);
  int h = std::abs(maxY - y);

  if ((!size.isEmpty()) && ((w < widthT) || (h < heightT))) {
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