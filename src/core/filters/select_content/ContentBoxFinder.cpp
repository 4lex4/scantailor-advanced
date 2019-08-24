// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ContentBoxFinder.h"
#include <Binarize.h>
#include <BinaryImage.h>
#include <ConnComp.h>
#include <ConnCompEraserExt.h>
#include <Connectivity.h>
#include <ConnectivityMap.h>
#include <GrayRasterOp.h>
#include <InfluenceMap.h>
#include <MaxWhitespaceFinder.h>
#include <Morphology.h>
#include <PolygonRasterizer.h>
#include <RasterOp.h>
#include <SEDM.h>
#include <SeedFill.h>
#include <SlicedHistogram.h>
#include <Transform.h>
#include <QDebug>
#include <QPainter>
#include <cmath>
#include <queue>
#include "DebugImages.h"
#include "Despeckle.h"
#include "FilterData.h"
#include "TaskStatus.h"

namespace select_content {
using namespace imageproc;

class ContentBoxFinder::Garbage {
 public:
  enum Type { HOR, VERT };

  Garbage(Type type, const BinaryImage& garbage);

  void add(const BinaryImage& garbage, const QRect& rect);

  const BinaryImage& image() const { return m_garbage; }

  const SEDM& sedm();

 private:
  imageproc::BinaryImage m_garbage;
  SEDM m_sedm;
  SEDM::Borders m_sedmBorders;
  bool m_sedmUpdatePending;
};


namespace {
struct PreferHorizontal {
  bool operator()(const QRect& lhs, const QRect& rhs) const {
    return lhs.width() * lhs.width() * lhs.height() < rhs.width() * rhs.width() * rhs.height();
  }
};

struct PreferVertical {
  bool operator()(const QRect& lhs, const QRect& rhs) const {
    return lhs.width() * lhs.height() * lhs.height() < rhs.width() * rhs.height() * rhs.height();
  }
};
}  // namespace

QRectF ContentBoxFinder::findContentBox(const TaskStatus& status,
                                        const FilterData& data,
                                        const QRectF& pageRect,
                                        DebugImages* dbg) {
  ImageTransformation xform150dpi(data.xform());
  xform150dpi.preScaleToDpi(Dpi(150, 150));

  if (xform150dpi.resultingRect().toRect().isEmpty()) {
    return QRectF();
  }

  const GrayImage dataGrayImage = data.grayImageBlackOnWhite();
  const uint8_t darkestGrayLevel = imageproc::darkestGrayLevel(dataGrayImage);
  const QColor outsideColor(darkestGrayLevel, darkestGrayLevel, darkestGrayLevel);

  QImage gray150(transformToGray(dataGrayImage, xform150dpi.transform(), xform150dpi.resultingRect().toRect(),
                                 OutsidePixels::assumeColor(outsideColor)));
  // Note that we fill new areas that appear as a result of
  // rotation with black, not white.  Filling them with white
  // may be bad for detecting the shadow around the page.
  if (dbg) {
    dbg->add(gray150, "gray150");
  }

  BinaryImage bw150(binarizeWolf(gray150, QSize(51, 51), 50));
  if (dbg) {
    dbg->add(bw150, "bw150");
  }

  const double xscale = 150.0 / data.xform().origDpi().horizontal();
  const double yscale = 150.0 / data.xform().origDpi().vertical();
  QRectF pageRect150(pageRect.left() * xscale, pageRect.top() * yscale, pageRect.right() * xscale,
                     pageRect.bottom() * yscale);
  PolygonRasterizer::fillExcept(bw150, BLACK, pageRect150, Qt::WindingFill);

  PolygonRasterizer::fillExcept(bw150, BLACK, xform150dpi.resultingPreCropArea(), Qt::WindingFill);
  if (dbg) {
    dbg->add(bw150, "page_mask_applied");
  }

  BinaryImage horShadowsSeed(openBrick(bw150, QSize(200, 14), BLACK));
  if (dbg) {
    dbg->add(horShadowsSeed, "horShadowsSeed");
  }

  status.throwIfCancelled();

  BinaryImage verShadowsSeed(openBrick(bw150, QSize(14, 300), BLACK));
  if (dbg) {
    dbg->add(verShadowsSeed, "verShadowsSeed");
  }

  status.throwIfCancelled();

  BinaryImage shadowsSeed(horShadowsSeed.release());
  rasterOp<RopOr<RopSrc, RopDst>>(shadowsSeed, verShadowsSeed);
  verShadowsSeed.release();
  if (dbg) {
    dbg->add(shadowsSeed, "shadowsSeed");
  }

  status.throwIfCancelled();

  BinaryImage dilated(dilateBrick(bw150, QSize(3, 3)));
  if (dbg) {
    dbg->add(dilated, "dilated");
  }

  status.throwIfCancelled();

  BinaryImage shadowsDilated(seedFill(shadowsSeed, dilated, CONN8));
  dilated.release();
  if (dbg) {
    dbg->add(shadowsDilated, "shadowsDilated");
  }

  status.throwIfCancelled();

  rasterOp<RopAnd<RopSrc, RopDst>>(shadowsDilated, bw150);
  BinaryImage garbage(shadowsDilated.release());
  if (dbg) {
    dbg->add(garbage, "shadows");
  }

  status.throwIfCancelled();

  filterShadows(status, garbage, dbg);
  if (dbg) {
    dbg->add(garbage, "filtered_shadows");
  }

  status.throwIfCancelled();

  BinaryImage content(bw150.release());
  rasterOp<RopSubtract<RopDst, RopSrc>>(content, garbage);
  if (dbg) {
    dbg->add(content, "content");
  }

  status.throwIfCancelled();

  Despeckle::Level despeckleLevel = Despeckle::NORMAL;

  BinaryImage despeckled(Despeckle::despeckle(content, Dpi(150, 150), despeckleLevel, status, dbg));
  if (dbg) {
    dbg->add(despeckled, "despeckled");
  }

  status.throwIfCancelled();

  BinaryImage contentBlocks(content.size(), BLACK);
  const int areaThreshold = std::min(content.width(), content.height());

  {
    MaxWhitespaceFinder horWsFinder(PreferHorizontal(), despeckled);

    for (int i = 0; i < 80; ++i) {
      QRect ws(horWsFinder.next(horWsFinder.MANUAL_OBSTACLES));
      if (ws.isNull()) {
        break;
      }
      if (ws.width() * ws.height() < areaThreshold) {
        break;
      }
      contentBlocks.fill(ws, WHITE);
      const int heightFraction = ws.height() / 5;
      ws.setTop(ws.top() + heightFraction);
      ws.setBottom(ws.bottom() - heightFraction);
      horWsFinder.addObstacle(ws);
    }
  }

  {
    MaxWhitespaceFinder vertWsFinder(PreferVertical(), despeckled);

    for (int i = 0; i < 40; ++i) {
      QRect ws(vertWsFinder.next(vertWsFinder.MANUAL_OBSTACLES));
      if (ws.isNull()) {
        break;
      }
      if (ws.width() * ws.height() < areaThreshold) {
        break;
      }
      contentBlocks.fill(ws, WHITE);
      const int widthFraction = ws.width() / 5;
      ws.setLeft(ws.left() + widthFraction);
      ws.setRight(ws.right() - widthFraction);
      vertWsFinder.addObstacle(ws);
    }
  }

  if (dbg) {
    dbg->add(contentBlocks, "contentBlocks");
  }

  trimContentBlocksInPlace(despeckled, contentBlocks);
  if (dbg) {
    dbg->add(contentBlocks, "initial_trimming");
  }

  // Do some more whitespace finding.  This should help us separate
  // blocks that don't belong together.
  {
    BinaryImage tmp(content);
    rasterOp<RopOr<RopNot<RopSrc>, RopDst>>(tmp, contentBlocks);
    MaxWhitespaceFinder wsFinder(tmp.release(), QSize(4, 4));

    for (int i = 0; i < 10; ++i) {
      QRect ws(wsFinder.next());
      if (ws.isNull()) {
        break;
      }
      if (ws.width() * ws.height() < areaThreshold) {
        break;
      }
      contentBlocks.fill(ws, WHITE);
    }
  }
  if (dbg) {
    dbg->add(contentBlocks, "more_whitespace");
  }

  trimContentBlocksInPlace(despeckled, contentBlocks);
  if (dbg) {
    dbg->add(contentBlocks, "more_trimming");
  }

  despeckled.release();

  inPlaceRemoveAreasTouchingBorders(contentBlocks, dbg);
  if (dbg) {
    dbg->add(contentBlocks, "except_bordering");
  }

  BinaryImage textMask(estimateTextMask(content, contentBlocks, dbg));
  if (dbg) {
    QImage textMaskVisualized(content.size(), QImage::Format_ARGB32_Premultiplied);
    textMaskVisualized.fill(0xffffffff);  // Opaque white.
    QPainter painter(&textMaskVisualized);

    QImage tmp(content.size(), QImage::Format_ARGB32_Premultiplied);
    tmp.fill(0xff64dd62);  // Opaque light green.
    tmp.setAlphaChannel(textMask.inverted().toQImage());
    painter.drawImage(QPoint(0, 0), tmp);

    tmp.fill(0xe0000000);  // Mostly transparent black.
    tmp.setAlphaChannel(content.inverted().toQImage());
    painter.drawImage(QPoint(0, 0), tmp);

    painter.end();

    dbg->add(textMaskVisualized, "textMask");
  }

  // Make textMask strore the actual content pixels that are text.
  rasterOp<RopAnd<RopSrc, RopDst>>(textMask, content);

  QRect contentRect(contentBlocks.contentBoundingBox());
  // Temporarily reuse horShadowsSeed and verShadowsSeed.
  // It's OK they are null.
  segmentGarbage(garbage, horShadowsSeed, verShadowsSeed, dbg);
  garbage.release();

  if (dbg) {
    dbg->add(horShadowsSeed, "initial_hor_garbage");
    dbg->add(verShadowsSeed, "initial_vert_garbage");
  }

  Garbage horGarbage(Garbage::HOR, horShadowsSeed.release());
  Garbage vertGarbage(Garbage::VERT, verShadowsSeed.release());

  enum Side { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };

  int sideMask = LEFT | RIGHT | TOP | BOTTOM;

  while (sideMask && !contentRect.isEmpty()) {
    QRect oldContentRect;

    if (sideMask & LEFT) {
      sideMask &= ~LEFT;
      oldContentRect = contentRect;
      contentRect = trimLeft(content, contentBlocks, textMask, contentRect, vertGarbage, dbg);

      status.throwIfCancelled();

      if (contentRect.isEmpty()) {
        break;
      }
      if (oldContentRect != contentRect) {
        sideMask |= LEFT | TOP | BOTTOM;
      }
    }

    if (sideMask & RIGHT) {
      sideMask &= ~RIGHT;
      oldContentRect = contentRect;
      contentRect = trimRight(content, contentBlocks, textMask, contentRect, vertGarbage, dbg);

      status.throwIfCancelled();

      if (contentRect.isEmpty()) {
        break;
      }
      if (oldContentRect != contentRect) {
        sideMask |= RIGHT | TOP | BOTTOM;
      }
    }

    if (sideMask & TOP) {
      sideMask &= ~TOP;
      oldContentRect = contentRect;
      contentRect = trimTop(content, contentBlocks, textMask, contentRect, horGarbage, dbg);

      status.throwIfCancelled();

      if (contentRect.isEmpty()) {
        break;
      }
      if (oldContentRect != contentRect) {
        sideMask |= TOP | LEFT | RIGHT;
      }
    }

    if (sideMask & BOTTOM) {
      sideMask &= ~BOTTOM;
      oldContentRect = contentRect;
      contentRect = trimBottom(content, contentBlocks, textMask, contentRect, horGarbage, dbg);

      status.throwIfCancelled();

      if (contentRect.isEmpty()) {
        break;
      }
      if (oldContentRect != contentRect) {
        sideMask |= BOTTOM | LEFT | RIGHT;
      }
    }

    if ((contentRect.width() < 8) || (contentRect.height() < 8)) {
      contentRect = QRect();
      break;
    } else if ((contentRect.width() < 30) && (contentRect.height() > contentRect.width() * 20)) {
      contentRect = QRect();
      break;
    }
  }

  // Increase the content rect due to cutting off the content at edges because of rescaling made.
  if (!contentRect.isEmpty()) {
    contentRect.adjust(-1, -1, 1, 1);
  }

  // Transform back from 150dpi.
  QTransform combinedXform(xform150dpi.transform().inverted());
  combinedXform *= data.xform().transform();

  return combinedXform.map(QRectF(contentRect)).boundingRect().intersected(data.xform().resultingRect());
}  // ContentBoxFinder::findContentBox

namespace {
struct Bounds {
  // All are inclusive.
  int left;
  int right;
  int top;
  int bottom;

  Bounds() : left(INT_MAX), right(INT_MIN), top(INT_MAX), bottom(INT_MIN) {}

  bool isInside(int x, int y) const {
    if (x < left) {
      return false;
    } else if (x > right) {
      return false;
    } else if (y < top) {
      return false;
    } else if (y > bottom) {
      return false;
    } else {
      return true;
    }
  }

  void forceInside(int x, int y) {
    if (x < left) {
      left = x;
    }
    if (x > right) {
      right = x;
    }
    if (y < top) {
      top = y;
    }
    if (y > bottom) {
      bottom = y;
    }
  }
};
}  // namespace

void ContentBoxFinder::trimContentBlocksInPlace(const imageproc::BinaryImage& content,
                                                imageproc::BinaryImage& contentBlocks) {
  const ConnectivityMap cmap(contentBlocks, CONN4);
  std::vector<Bounds> bounds(cmap.maxLabel() + 1);

  int width = content.width();
  int height = content.height();
  const uint32_t msb = uint32_t(1) << 31;

  const uint32_t* contentLine = content.data();
  const int contentStride = content.wordsPerLine();
  const uint32_t* cmapLine = cmap.data();
  const int cmapStride = cmap.stride();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = cmapLine[x];
      if (label == 0) {
        continue;
      }
      if (contentLine[x >> 5] & (msb >> (x & 31))) {
        bounds[label].forceInside(x, y);
      }
    }
    cmapLine += cmapStride;
    contentLine += contentStride;
  }

  uint32_t* cbLine = contentBlocks.data();
  const int cbStride = contentBlocks.wordsPerLine();
  cmapLine = cmap.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = cmapLine[x];
      if (label == 0) {
        continue;
      }
      if (!bounds[label].isInside(x, y)) {
        cbLine[x >> 5] &= ~(msb >> (x & 31));
      }
    }
    cmapLine += cmapStride;
    cbLine += cbStride;
  }
}  // ContentBoxFinder::trimContentBlocksInPlace

void ContentBoxFinder::inPlaceRemoveAreasTouchingBorders(imageproc::BinaryImage& contentBlocks, DebugImages* dbg) {
  // We could just do a seed fill from borders, but that
  // has the potential to remove too much.  Instead, we
  // do something similar to a seed fill, but with a limited
  // spread distance.


  const int width = contentBlocks.width();
  const int height = contentBlocks.height();

  const auto maxSpreadDist = static_cast<uint16_t>(std::min(width, height) / 4);

  std::vector<uint16_t> map((width + 2) * (height + 2), ~uint16_t(0));

  uint32_t* cbLine = contentBlocks.data();
  const int cbStride = contentBlocks.wordsPerLine();
  uint16_t* mapLine = &map[0] + width + 3;
  const int mapStride = width + 2;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t mask = cbLine[x >> 5] >> (31 - (x & 31));
      mask &= uint32_t(1);
      --mask;

      // WHITE -> max, BLACK -> 0
      mapLine[x] = static_cast<uint16_t>(mask);
    }
    mapLine += mapStride;
    cbLine += cbStride;
  }

  std::queue<uint16_t*> queue;
  // Initialize border seeds.
  mapLine = &map[0] + width + 3;
  for (int x = 0; x < width; ++x) {
    if (mapLine[x] == 0) {
      mapLine[x] = maxSpreadDist;
      queue.push(&mapLine[x]);
    }
  }
  for (int y = 1; y < height - 1; ++y) {
    if (mapLine[0] == 0) {
      mapLine[0] = maxSpreadDist;
      queue.push(&mapLine[0]);
    }
    if (mapLine[width - 1] == 0) {
      mapLine[width - 1] = maxSpreadDist;
      queue.push(&mapLine[width - 1]);
    }
    mapLine += mapStride;
  }
  for (int x = 0; x < width; ++x) {
    if (mapLine[x] == 0) {
      mapLine[x] = maxSpreadDist;
      queue.push(&mapLine[x]);
    }
  }

  if (queue.empty()) {
    // Common case optimization.
    return;
  }

  while (!queue.empty()) {
    uint16_t* cell = queue.front();
    queue.pop();

    assert(*cell != 0);
    const auto newDist = static_cast<uint16_t>(*cell - 1);

    uint16_t* nbh = cell - mapStride;
    if (newDist > *nbh) {
      *nbh = newDist;
      queue.push(nbh);
    }

    nbh = cell - 1;
    if (newDist > *nbh) {
      *nbh = newDist;
      queue.push(nbh);
    }

    nbh = cell + 1;
    if (newDist > *nbh) {
      *nbh = newDist;
      queue.push(nbh);
    }

    nbh = cell + mapStride;
    if (newDist > *nbh) {
      *nbh = newDist;
      queue.push(nbh);
    }
  }

  cbLine = contentBlocks.data();
  mapLine = &map[0] + width + 3;
  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mapLine[x] + 1 > 1) {  // If not 0 or ~uint16_t(0)
        cbLine[x >> 5] &= ~(msb >> (x & 31));
      }
    }
    mapLine += mapStride;
    cbLine += cbStride;
  }
}  // ContentBoxFinder::inPlaceRemoveAreasTouchingBorders

void ContentBoxFinder::segmentGarbage(const imageproc::BinaryImage& garbage,
                                      imageproc::BinaryImage& horGarbage,
                                      imageproc::BinaryImage& vertGarbage,
                                      DebugImages* dbg) {
  horGarbage = openBrick(garbage, QSize(200, 1), WHITE);

  QRect rect(garbage.rect());
  rect.setHeight(1);
  rasterOp<RopOr<RopSrc, RopDst>>(horGarbage, rect, garbage, rect.topLeft());
  rect.moveBottom(garbage.rect().bottom());
  rasterOp<RopOr<RopSrc, RopDst>>(horGarbage, rect, garbage, rect.topLeft());

  vertGarbage = openBrick(garbage, QSize(1, 200), WHITE);

  rect = garbage.rect();
  rect.setWidth(1);
  rasterOp<RopOr<RopSrc, RopDst>>(vertGarbage, rect, garbage, rect.topLeft());
  rect.moveRight(garbage.rect().right());
  rasterOp<RopOr<RopSrc, RopDst>>(vertGarbage, rect, garbage, rect.topLeft());

  ConnectivityMap cmap(garbage.size());

  cmap.addComponent(vertGarbage);
  vertGarbage.fill(WHITE);
  cmap.addComponent(horGarbage);
  horGarbage.fill(WHITE);

  InfluenceMap imap(cmap, garbage);
  cmap = ConnectivityMap();

  const int width = garbage.width();
  const int height = garbage.height();

  InfluenceMap::Cell* imapLine = imap.data();
  const int imapStride = imap.stride();

  uint32_t* vgLine = vertGarbage.data();
  const int vgStride = vertGarbage.wordsPerLine();
  uint32_t* hgLine = horGarbage.data();
  const int hgStride = horGarbage.wordsPerLine();

  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      switch (imapLine[x].label) {
        case 1:
          vgLine[x >> 5] |= msb >> (x & 31);
          break;
        case 2:
          hgLine[x >> 5] |= msb >> (x & 31);
          break;
        default:
          break;
      }
    }
    imapLine += imapStride;
    vgLine += vgStride;
    hgLine += hgStride;
  }

  BinaryImage unconnectedGarbage(garbage);
  rasterOp<RopSubtract<RopDst, RopSrc>>(unconnectedGarbage, horGarbage);
  rasterOp<RopSubtract<RopDst, RopSrc>>(unconnectedGarbage, vertGarbage);

  rasterOp<RopOr<RopSrc, RopDst>>(horGarbage, unconnectedGarbage);
  rasterOp<RopOr<RopSrc, RopDst>>(vertGarbage, unconnectedGarbage);
}  // ContentBoxFinder::segmentGarbage

imageproc::BinaryImage ContentBoxFinder::estimateTextMask(const imageproc::BinaryImage& content,
                                                          const imageproc::BinaryImage& contentBlocks,
                                                          DebugImages* dbg) {
  // We differentiate between a text line and a slightly skewed straight
  // line (which may have a fill factor similar to that of text) by the
  // presence of ultimate eroded points.

  const BinaryImage ueps(SEDM(content, SEDM::DIST_TO_BLACK, SEDM::DIST_TO_NO_BORDERS).findPeaksDestructive());
  if (dbg) {
    QImage canvas(contentBlocks.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
    QPainter painter;
    painter.begin(&canvas);
    QImage overlay(canvas.size(), canvas.format());
    overlay.fill(0xff0000ff);  // opaque blue
    overlay.setAlphaChannel(content.inverted().toQImage());
    painter.drawImage(QPoint(0, 0), overlay);

    BinaryImage uepsOnContentBlocks(contentBlocks);
    rasterOp<RopAnd<RopSrc, RopDst>>(uepsOnContentBlocks, ueps);

    overlay.fill(0xffffff00);  // opaque yellow
    overlay.setAlphaChannel(uepsOnContentBlocks.inverted().toQImage());
    painter.drawImage(QPoint(0, 0), overlay);

    painter.end();
    dbg->add(canvas, "ueps");
  }

  BinaryImage textMask(content.size(), WHITE);

  const int minTextHeight = 6;

  ConnCompEraserExt eraser(contentBlocks, CONN4);
  while (true) {
    const ConnComp cc(eraser.nextConnComp());
    if (cc.isNull()) {
      break;
    }

    BinaryImage ccImg(eraser.computeConnCompImage());
    BinaryImage contentImg(ccImg.size());
    rasterOp<RopSrc>(contentImg, contentImg.rect(), content, cc.rect().topLeft());

    // Note that some content may actually be not masked
    // by contentBlocks, because we build contentBlocks
    // based on despeckled content image.
    rasterOp<RopAnd<RopSrc, RopDst>>(contentImg, ccImg);

    const SlicedHistogram hist(contentImg, SlicedHistogram::ROWS);
    const SlicedHistogram blockHist(ccImg, SlicedHistogram::ROWS);

    assert(hist.size() != 0);

    using Range = std::pair<const int*, const int*>;
    std::vector<Range> ranges;
    std::vector<Range> splittableRanges;
    splittableRanges.emplace_back(&hist[0], &hist[hist.size() - 1]);

    std::vector<int> maxForward(hist.size());
    std::vector<int> maxBackwards(hist.size());

    // Try splitting text lines.
    while (!splittableRanges.empty()) {
      const int* const first = splittableRanges.back().first;
      const int* const last = splittableRanges.back().second;
      splittableRanges.pop_back();

      if (last - first < minTextHeight - 1) {
        // Just ignore such a small segment.
        continue;
      }
      // Fill maxForward and maxBackwards.
      {
        int prev = *first;
        for (int i = 0; i <= last - first; ++i) {
          prev = std::max(prev, first[i]);
          maxForward[i] = prev;
        }
        prev = *last;
        for (int i = 0; i <= last - first; ++i) {
          prev = std::max(prev, last[-i]);
          maxBackwards[i] = prev;
        }
      }

      int bestMagnitude = std::numeric_limits<int>::min();
      const int* bestSplitPos = nullptr;
      assert(first != last);
      for (const int* p = first + 1; p != last; ++p) {
        const int peak1 = maxForward[p - (first + 1)];
        const int peak2 = maxBackwards[(last - 1) - p];
        if (*p * 3.5 > 0.5 * (peak1 + peak2)) {
          continue;
        }
        const int shoulder1 = peak1 - *p;
        const int shoulder2 = peak2 - *p;
        if ((shoulder1 <= 0) || (shoulder2 <= 0)) {
          continue;
        }
        if (std::min(shoulder1, shoulder2) * 20 < std::max(shoulder1, shoulder2)) {
          continue;
        }

        const int magnitude = shoulder1 + shoulder2;
        if (magnitude > bestMagnitude) {
          bestMagnitude = magnitude;
          bestSplitPos = p;
        }
      }

      if (bestSplitPos) {
        splittableRanges.emplace_back(first, bestSplitPos - 1);
        splittableRanges.emplace_back(bestSplitPos + 1, last);
      } else {
        ranges.emplace_back(first, last);
      }
    }

    for (const Range range : ranges) {
      const auto first = static_cast<int>(range.first - &hist[0]);
      const auto last = static_cast<int>(range.second - &hist[0]);
      if (last - first < minTextHeight - 1) {
        continue;
      }

      int64_t weightedY = 0;
      int totalWeight = 0;
      for (int i = first; i <= last; ++i) {
        const int val = hist[i];
        weightedY += val * i;
        totalWeight += val;
      }

      if (totalWeight == 0) {
        // qDebug() << "no black pixels at all";
        continue;
      }

      const double minFillFactor = 0.22;
      const double maxFillFactor = 0.65;

      const auto centerY = static_cast<int>((weightedY + totalWeight / 2) / totalWeight);
      int top = centerY - minTextHeight / 2;
      int bottom = top + minTextHeight - 1;
      int numBlack = 0;
      int numTotal = 0;
      int maxWidth = 0;
      if ((top < first) || (bottom > last)) {
        continue;
      }
      for (int i = top; i <= bottom; ++i) {
        numBlack += hist[i];
        numTotal += blockHist[i];
        maxWidth = std::max(maxWidth, blockHist[i]);
      }
      if (numBlack < numTotal * minFillFactor) {
        // qDebug() << "initial fill factor too low";
        continue;
      }
      if (numBlack > numTotal * maxFillFactor) {
        // qDebug() << "initial fill factor too high";
        continue;
      }

      // Extend the top and bottom of the text line.
      while ((top > first || bottom < last) && std::abs((centerY - top) - (bottom - centerY)) <= 1) {
        const int newTop = (top > first) ? top - 1 : top;
        const int newBottom = (bottom < last) ? bottom + 1 : bottom;
        numBlack += hist[newTop] + hist[newBottom];
        numTotal += blockHist[newTop] + blockHist[newBottom];
        if (numBlack < numTotal * minFillFactor) {
          break;
        }
        maxWidth = std::max(maxWidth, blockHist[newTop]);
        maxWidth = std::max(maxWidth, blockHist[newBottom]);
        top = newTop;
        bottom = newBottom;
      }

      if (numBlack > numTotal * maxFillFactor) {
        // qDebug() << "final fill factor too high";
        continue;
      }

      if (maxWidth < (bottom - top + 1) * 0.6) {
        // qDebug() << "aspect ratio too low";
        continue;
      }

      QRect lineRect(cc.rect());
      lineRect.setTop(cc.rect().top() + top);
      lineRect.setBottom(cc.rect().top() + bottom);

      // Check if there are enough ultimate eroded points on the line.
      auto uepsTodo = int(0.4 * lineRect.width() / lineRect.height());
      if (uepsTodo) {
        BinaryImage lineUeps(lineRect.size());
        rasterOp<RopSrc>(lineUeps, lineUeps.rect(), contentBlocks, lineRect.topLeft());
        rasterOp<RopAnd<RopSrc, RopDst>>(lineUeps, lineUeps.rect(), ueps, lineRect.topLeft());
        ConnCompEraser uepsEraser(lineUeps, CONN4);
        ConnComp cc;
        for (; uepsTodo && !(cc = uepsEraser.nextConnComp()).isNull(); --uepsTodo) {
          // Erase components until uepsTodo reaches zero or there are no more components.
        }
        if (uepsTodo) {
          // Not enough ueps were found.
          // qDebug() << "Not enough UEPs.";
          continue;
        }
      }
      // Write this block to the text mask.
      rasterOp<RopOr<RopSrc, RopDst>>(textMask, lineRect, ccImg, QPoint(0, top));
    }
  }

  return textMask;
}  // ContentBoxFinder::estimateTextMask

QRect ContentBoxFinder::trimLeft(const imageproc::BinaryImage& content,
                                 const imageproc::BinaryImage& contentBlocks,
                                 const imageproc::BinaryImage& text,
                                 const QRect& area,
                                 Garbage& garbage,
                                 DebugImages* const dbg) {
  const SlicedHistogram hist(contentBlocks, area, SlicedHistogram::COLS);

  size_t start = 0;
  while (start < hist.size()) {
    size_t firstWs = start;
    for (; firstWs < hist.size() && hist[firstWs] != 0; ++firstWs) {
      // Skip non-empty columns.
    }
    size_t firstNonWs = firstWs;
    for (; firstNonWs < hist.size() && hist[firstNonWs] == 0; ++firstNonWs) {
      // Skip empty columns.
    }

    firstWs += area.left();
    firstNonWs += area.left();

    QRect newArea(area);
    newArea.setLeft(static_cast<int>(firstNonWs));
    if (newArea.isEmpty()) {
      return area;
    }

    QRect removedArea(area);
    removedArea.setRight(static_cast<int>(firstWs - 1));
    if (removedArea.isEmpty()) {
      return newArea;
    }

    bool canRetryGrouped = false;
    const QRect res = trim(content, contentBlocks, text, area, newArea, removedArea, garbage, canRetryGrouped, dbg);
    if (canRetryGrouped) {
      start = firstNonWs - area.left();
    } else {
      return res;
    }
  }

  return area;
}  // ContentBoxFinder::trimLeft

QRect ContentBoxFinder::trimRight(const imageproc::BinaryImage& content,
                                  const imageproc::BinaryImage& contentBlocks,
                                  const imageproc::BinaryImage& text,
                                  const QRect& area,
                                  Garbage& garbage,
                                  DebugImages* const dbg) {
  const SlicedHistogram hist(contentBlocks, area, SlicedHistogram::COLS);

  auto start = static_cast<int>(hist.size() - 1);
  while (start >= 0) {
    int firstWs = start;
    for (; firstWs >= 0 && hist[firstWs] != 0; --firstWs) {
      // Skip non-empty columns.
    }
    int firstNonWs = firstWs;
    for (; firstNonWs >= 0 && hist[firstNonWs] == 0; --firstNonWs) {
      // Skip empty columns.
    }

    firstWs += area.left();
    firstNonWs += area.left();

    QRect newArea(area);
    newArea.setRight(firstNonWs);
    if (newArea.isEmpty()) {
      return area;
    }

    QRect removedArea(area);
    removedArea.setLeft(firstWs + 1);
    if (removedArea.isEmpty()) {
      return newArea;
    }

    bool canRetryGrouped = false;
    const QRect res = trim(content, contentBlocks, text, area, newArea, removedArea, garbage, canRetryGrouped, dbg);
    if (canRetryGrouped) {
      start = firstNonWs - area.left();
    } else {
      return res;
    }
  }

  return area;
}  // ContentBoxFinder::trimRight

QRect ContentBoxFinder::trimTop(const imageproc::BinaryImage& content,
                                const imageproc::BinaryImage& contentBlocks,
                                const imageproc::BinaryImage& text,
                                const QRect& area,
                                Garbage& garbage,
                                DebugImages* const dbg) {
  const SlicedHistogram hist(contentBlocks, area, SlicedHistogram::ROWS);

  size_t start = 0;
  while (start < hist.size()) {
    size_t firstWs = start;
    for (; firstWs < hist.size() && hist[firstWs] != 0; ++firstWs) {
      // Skip non-empty columns.
    }
    size_t firstNonWs = firstWs;
    for (; firstNonWs < hist.size() && hist[firstNonWs] == 0; ++firstNonWs) {
      // Skip empty columns.
    }

    firstWs += area.top();
    firstNonWs += area.top();

    QRect newArea(area);
    newArea.setTop(static_cast<int>(firstNonWs));
    if (newArea.isEmpty()) {
      return area;
    }

    QRect removedArea(area);
    removedArea.setBottom(static_cast<int>(firstWs - 1));
    if (removedArea.isEmpty()) {
      return newArea;
    }

    bool canRetryGrouped = false;
    const QRect res = trim(content, contentBlocks, text, area, newArea, removedArea, garbage, canRetryGrouped, dbg);
    if (canRetryGrouped) {
      start = firstNonWs - area.top();
    } else {
      return res;
    }
  }

  return area;
}  // ContentBoxFinder::trimTop

QRect ContentBoxFinder::trimBottom(const imageproc::BinaryImage& content,
                                   const imageproc::BinaryImage& contentBlocks,
                                   const imageproc::BinaryImage& text,
                                   const QRect& area,
                                   Garbage& garbage,
                                   DebugImages* const dbg) {
  const SlicedHistogram hist(contentBlocks, area, SlicedHistogram::ROWS);

  auto start = static_cast<int>(hist.size() - 1);
  while (start >= 0) {
    int firstWs = start;
    for (; firstWs >= 0 && hist[firstWs] != 0; --firstWs) {
      // Skip non-empty columns.
    }
    int firstNonWs = firstWs;
    for (; firstNonWs >= 0 && hist[firstNonWs] == 0; --firstNonWs) {
      // Skip empty columns.
    }

    firstWs += area.top();
    firstNonWs += area.top();

    QRect newArea(area);
    newArea.setBottom(firstNonWs);
    if (newArea.isEmpty()) {
      return area;
    }

    QRect removedArea(area);
    removedArea.setTop(firstWs + 1);
    if (removedArea.isEmpty()) {
      return newArea;
    }

    bool canRetryGrouped = false;
    const QRect res = trim(content, contentBlocks, text, area, newArea, removedArea, garbage, canRetryGrouped, dbg);
    if (canRetryGrouped) {
      start = firstNonWs - area.top();
    } else {
      return res;
    }
  }

  return area;
}  // ContentBoxFinder::trimBottom

QRect ContentBoxFinder::trim(const imageproc::BinaryImage& content,
                             const imageproc::BinaryImage& contentBlocks,
                             const imageproc::BinaryImage& text,
                             const QRect& area,
                             const QRect& newArea,
                             const QRect& removedArea,
                             Garbage& garbage,
                             bool& canRetryGrouped,
                             DebugImages* const dbg) {
  canRetryGrouped = false;

  QImage visualized;

  if (dbg) {
    visualized = QImage(contentBlocks.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&visualized);
    painter.drawImage(QPoint(0, 0), contentBlocks.toQImage());

    QPainterPath outerPath;
    outerPath.addRect(visualized.rect());
    QPainterPath innerPath;
    innerPath.addRect(area);

    // Fill already rejected area with translucent gray.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x00, 0x00, 0x00, 50));
    painter.drawPath(outerPath.subtracted(innerPath));
  }

  // Don't trim too much.
  while (removedArea.width() * removedArea.height() > 0.3 * (newArea.width() * newArea.height())) {
    // It's a loop just to be able to break from it.

    // There is a special case when there is nothing but
    // garbage on the page.  Let's try to handle it here.
    if ((removedArea.width() < 6) || (removedArea.height() < 6)) {
      break;
    }

    if (dbg) {
      QPainter painter(&visualized);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(0x5f, 0xdf, 0x57, 50));
      painter.drawRect(removedArea);
      painter.drawRect(newArea);
      painter.end();
      dbg->add(visualized, "trim_too_much");
    }

    return area;
  }

  const int contentPixels = content.countBlackPixels(removedArea);

  const bool verticalCut = (newArea.top() == area.top() && newArea.bottom() == area.bottom());
  // qDebug() << "vertical cut: " << verticalCut;
  // Ranged from 0.0 to 1.0.  When it's less than 0.5, objects
  // are more likely to be considered as garbage.  When it's
  // more than 0.5, objects are less likely to be considered
  // as garbage.
  double proximityBias = verticalCut ? 0.5 : 0.65;

  const int numTextPixels = text.countBlackPixels(removedArea);
  if (numTextPixels == 0) {
    proximityBias = verticalCut ? 0.4 : 0.5;
  } else {
    int totalPixels = contentPixels;
    totalPixels += garbage.image().countBlackPixels(removedArea);

    // qDebug() << "numTextPixels = " << numTextPixels;
    // qDebug() << "totalPixels = " << totalPixels;
    ++totalPixels;  // just in case
    const double minTextInfluence = 0.2;
    const double maxTextInfluence = 1.0;
    const int upperThreshold = 5000;
    double textInfluence = maxTextInfluence;
    if (numTextPixels < upperThreshold) {
      textInfluence = minTextInfluence
                      + (maxTextInfluence - minTextInfluence) * std::log((double) numTextPixels)
                            / std::log((double) upperThreshold);
    }
    // qDebug() << "textInfluence = " << textInfluence;

    proximityBias += (1.0 - proximityBias) * textInfluence * numTextPixels / totalPixels;
    proximityBias = qBound(0.0, proximityBias, 1.0);
  }

  BinaryImage remainingContent(contentBlocks.size(), WHITE);
  rasterOp<RopSrc>(remainingContent, newArea, content, newArea.topLeft());
  rasterOp<RopAnd<RopSrc, RopDst>>(remainingContent, newArea, contentBlocks, newArea.topLeft());

  const SEDM dmToOthers(remainingContent, SEDM::DIST_TO_BLACK, SEDM::DIST_TO_NO_BORDERS);
  remainingContent.release();

  double sumDistToGarbage = 0;
  double sumDistToOthers = 0;

  const uint32_t* cbLine = contentBlocks.data();
  const int cbStride = contentBlocks.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  const uint32_t* dmGarbageLine = garbage.sedm().data();
  const uint32_t* dmOthersLine = dmToOthers.data();
  const int dmStride = dmToOthers.stride();

  int count = 0;
  cbLine += cbStride * removedArea.top();
  dmGarbageLine += dmStride * removedArea.top();
  dmOthersLine += dmStride * removedArea.top();
  for (int y = removedArea.top(); y <= removedArea.bottom(); ++y) {
    for (int x = removedArea.left(); x <= removedArea.right(); ++x) {
      if (cbLine[x >> 5] & (msb >> (x & 31))) {
        sumDistToGarbage += std::sqrt((double) dmGarbageLine[x]);
        sumDistToOthers += std::sqrt((double) dmOthersLine[x]);
        ++count;
      }
    }
    cbLine += cbStride;
    dmGarbageLine += dmStride;
    dmOthersLine += dmStride;
  }

  // qDebug() << "proximityBias = " << proximityBias;
  // qDebug() << "sumDistToGarbage = " << sumDistToGarbage;
  // qDebug() << "sumDistToOthers = " << sumDistToOthers;
  // qDebug() << "count = " << count;

  sumDistToGarbage *= proximityBias;
  sumDistToOthers *= 1.0 - proximityBias;

  if (sumDistToGarbage < sumDistToOthers) {
    garbage.add(content, removedArea);
    if (dbg) {
      QPainter painter(&visualized);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(0x5f, 0xdf, 0x57, 50));
      painter.drawRect(newArea);
      painter.setBrush(QColor(0xff, 0x20, 0x1e, 50));
      painter.drawRect(removedArea);
      painter.end();
      dbg->add(visualized, "trimmed");
    }

    return newArea;
  } else {
    if (dbg) {
      QPainter painter(&visualized);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(0x5f, 0xdf, 0x57, 50));
      painter.drawRect(removedArea);
      painter.drawRect(newArea);
      painter.end();
      dbg->add(visualized, "not_trimmed");
    }
    canRetryGrouped = (proximityBias < 0.85);

    return area;
  }
}  // ContentBoxFinder::trim

void ContentBoxFinder::filterShadows(const TaskStatus& status,
                                     imageproc::BinaryImage& shadows,
                                     DebugImages* const dbg) {
  // The input image should only contain shadows from the edges
  // of a page, but in practice it may also contain things like
  // a black table header which white letters on it.  Here we
  // try to filter them out.

#if 1
  // Shadows that touch borders are genuine and should not be removed.
  BinaryImage borders(shadows.size(), WHITE);
  borders.fillExcept(borders.rect().adjusted(1, 1, -1, -1), BLACK);

  BinaryImage touchingShadows(seedFill(borders, shadows, CONN8));
  rasterOp<RopXor<RopSrc, RopDst>>(shadows, touchingShadows);
  if (dbg) {
    dbg->add(shadows, "non_border_shadows");
  }

  if (shadows.countBlackPixels()) {
    BinaryImage invShadows(shadows.inverted());
    BinaryImage mask(seedFill(borders, invShadows, CONN8));
    borders.release();
    rasterOp<RopOr<RopNot<RopDst>, RopSrc>>(mask, shadows);
    if (dbg) {
      dbg->add(mask, "shadows_no_holes");
    }

    BinaryImage textMask(estimateTextMask(invShadows, mask, dbg));
    invShadows.release();
    mask.release();
    textMask = seedFill(textMask, shadows, CONN8);
    if (dbg) {
      dbg->add(textMask, "misclassified_shadows");
    }
    rasterOp<RopXor<RopSrc, RopDst>>(shadows, textMask);
  }

  rasterOp<RopOr<RopSrc, RopDst>>(shadows, touchingShadows);
#else   // if 1
        // White dots on black background may be a problem for us.
        // They may be misclassified as parts of white letters.
  BinaryImage reducedDithering(closeBrick(shadows, QSize(1, 2), BLACK));
  reducedDithering = closeBrick(reducedDithering, QSize(2, 1), BLACK);
  if (dbg) {
    dbg->add(reducedDithering, "reducedDithering");
  }

  status.throwIfCancelled();

  // Long white vertical lines are definately not spaces between letters.
  BinaryImage vertWhitespace(closeBrick(reducedDithering, QSize(1, 150), BLACK));
  if (dbg) {
    dbg->add(vertWhitespace, "vertWhitespace");
  }

  status.throwIfCancelled();

  // Join neighboring white letters.
  BinaryImage opened(openBrick(reducedDithering, QSize(10, 4), BLACK));
  reducedDithering.release();
  if (dbg) {
    dbg->add(opened, "opened");
  }

  status.throwIfCancelled();

  // Extract areas that became white as a result of the last operation.
  rasterOp<RopSubtract<RopNot<RopDst>, RopNot<RopSrc>>>(opened, shadows);
  if (dbg) {
    dbg->add(opened, "became white");
  }

  status.throwIfCancelled();
  // Join the spacings between words together.
  BinaryImage closed(closeBrick(opened, QSize(20, 1), WHITE));
  opened.release();
  rasterOp<RopAnd<RopSrc, RopDst>>(closed, vertWhitespace);
  vertWhitespace.release();
  if (dbg) {
    dbg->add(closed, "closed");
  }

  status.throwIfCancelled();
  // If we've got long enough and tall enough blocks, we assume they
  // are the text lines.
  opened = openBrick(closed, QSize(50, 10), WHITE);
  closed.release();
  if (dbg) {
    dbg->add(opened, "reopened");
  }

  status.throwIfCancelled();

  BinaryImage nonShadows(seedFill(opened, shadows, CONN8));
  opened.release();
  if (dbg) {
    dbg->add(nonShadows, "nonShadows");
  }

  status.throwIfCancelled();

  rasterOp<RopSubtract<RopDst, RopSrc>>(shadows, nonShadows);
#endif  // if 1
}  // ContentBoxFinder::filterShadows

/*====================== ContentBoxFinder::Garbage =====================*/

ContentBoxFinder::Garbage::Garbage(const Type type, const BinaryImage& garbage)
    : m_garbage(garbage),
      m_sedmBorders(type == VERT ? SEDM::DIST_TO_VERT_BORDERS : SEDM::DIST_TO_HOR_BORDERS),
      m_sedmUpdatePending(true) {}

void ContentBoxFinder::Garbage::add(const BinaryImage& garbage, const QRect& rect) {
  rasterOp<RopOr<RopSrc, RopDst>>(m_garbage, rect, garbage, rect.topLeft());
  m_sedmUpdatePending = true;
}

const SEDM& ContentBoxFinder::Garbage::sedm() {
  if (m_sedmUpdatePending) {
    m_sedm = SEDM(m_garbage, SEDM::DIST_TO_BLACK, m_sedmBorders);
  }

  return m_sedm;
}
}  // namespace select_content