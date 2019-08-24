// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageLayoutEstimator.h"
#include <Binarize.h>
#include <BinaryThreshold.h>
#include <ConnComp.h>
#include <ConnCompEraserExt.h>
#include <Connectivity.h>
#include <Constants.h>
#include <GrayRasterOp.h>
#include <Grayscale.h>
#include <Morphology.h>
#include <PolygonRasterizer.h>
#include <RasterOp.h>
#include <ReduceThreshold.h>
#include <Scale.h>
#include <SeedFill.h>
#include <Shear.h>
#include <SkewFinder.h>
#include <SlicedHistogram.h>
#include <Transform.h>
#include <imageproc/OrthogonalRotation.h>
#include <QDebug>
#include <QPainter>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "ContentSpanFinder.h"
#include "DebugImages.h"
#include "ImageMetadata.h"
#include "ImageTransformation.h"
#include "OrthogonalRotation.h"
#include "PageLayout.h"
#include "ProjectPages.h"
#include "VertLineFinder.h"

namespace page_split {
using namespace imageproc;

namespace {
double lineCenterX(const QLineF& line) {
  return 0.5 * (line.p1().x() + line.p2().x());
}

struct CenterComparator {
  bool operator()(const QLineF& line1, const QLineF& line2) const { return lineCenterX(line1) < lineCenterX(line2); }
};

/**
 * \brief Try to auto-detect a page layout for a single-page configuration.
 *
 * \param layoutType The requested layout type.  The layout type of
 *        SINGLE_PAGE_UNCUT is not handled here.
 * \param ltrLines Folding line candidates sorted from left to right.
 * \param imageSize The dimentions of the page image.
 * \param hor_shadows A downscaled grayscale image that contains
 *        long enough and not too thin horizontal lines.
 * \param dbg An optional sink for debugging images.
 * \return The page layout detected or a null unique_ptr.
 */
std::unique_ptr<PageLayout> autoDetectSinglePageLayout(const LayoutType layoutType,
                                                       const std::vector<QLineF>& ltrLines,
                                                       const QRectF& virtualImageRect,
                                                       const GrayImage& grayDownscaled,
                                                       const QTransform& outToDownscaled,
                                                       DebugImages* dbg) {
  const double imageCenter = virtualImageRect.center().x();

  // A loop just to be able to break from it.
  do {
    // This whole branch (loop) leads to SINGLE_PAGE_UNCUT,
    // which conflicts with PAGE_PLUS_OFFCUT.
    if (layoutType == PAGE_PLUS_OFFCUT) {
      break;
    }
    // If we have a single line close to an edge,
    // Or more than one line, with the first and the last
    // ones close to an edge, that looks more like
    // SINGLE_PAGE_CUT layout.
    if (!ltrLines.empty()) {
      const QLineF& firstLine = ltrLines.front();
      const double lineCenter = lineCenterX(firstLine);
      if (std::fabs(imageCenter - lineCenter) > 0.65 * imageCenter) {
        break;
      }
    }
    if (ltrLines.size() > 1) {
      const QLineF& lastLine = ltrLines.back();
      const double lineCenter = lineCenterX(lastLine);
      if (std::fabs(imageCenter - lineCenter) > 0.65 * imageCenter) {
        break;
      }
    }

    // Return a SINGLE_PAGE_UNCUT layout.
    return std::make_unique<PageLayout>(virtualImageRect);
  } while (false);

  if (ltrLines.empty()) {
    // Impossible to detect the layout type.
    return nullptr;
  } else if (ltrLines.size() > 1) {
    return std::make_unique<PageLayout>(virtualImageRect, ltrLines.front(), ltrLines.back());
  } else {
    assert(ltrLines.size() == 1);
    const QLineF& line = ltrLines.front();
    const double lineCenter = lineCenterX(line);
    if (lineCenter < imageCenter) {
      const QLineF rightLine(virtualImageRect.topRight(), virtualImageRect.bottomRight());

      return std::make_unique<PageLayout>(virtualImageRect, line, rightLine);
    } else {
      const QLineF leftLine(virtualImageRect.topLeft(), virtualImageRect.bottomLeft());

      return std::make_unique<PageLayout>(virtualImageRect, leftLine, line);
    }
  }
}  // autoDetectSinglePageLayout

/**
 * \brief Try to auto-detect a page layout for a two-page configuration.
 *
 * \param ltrLines Folding line candidates sorted from left to right.
 * \param imageSize The dimentions of the page image.
 * \return The page layout detected or a null unique_ptr.
 */
std::unique_ptr<PageLayout> autoDetectTwoPageLayout(const std::vector<QLineF>& ltrLines,
                                                    const QRectF& virtualImageRect) {
  if (ltrLines.empty()) {
    // Impossible to detect the page layout.
    return nullptr;
  } else if (ltrLines.size() == 1) {
    return std::make_unique<PageLayout>(virtualImageRect, ltrLines.front());
  }

  // Find the line closest to the center.
  const double imageCenter = virtualImageRect.center().x();
  double minDistance = std::numeric_limits<double>::max();
  const QLineF* bestLine = nullptr;
  for (const QLineF& line : ltrLines) {
    const double lineCenter = lineCenterX(line);
    const double distance = std::fabs(lineCenter - imageCenter);
    if (distance < minDistance) {
      minDistance = distance;
      bestLine = &line;
    }
  }

  return std::make_unique<PageLayout>(virtualImageRect, *bestLine);
}

int numPages(const LayoutType layoutType, const ImageTransformation& preXform) {
  int numPages = 0;

  switch (layoutType) {
    case AUTO_LAYOUT_TYPE: {
      const QSize imageSize(preXform.origRect().size().toSize());
      numPages = ProjectPages::adviseNumberOfLogicalPages(ImageMetadata(imageSize, preXform.origDpi()),
                                                          preXform.preRotation());
      break;
    }
    case SINGLE_PAGE_UNCUT:
    case PAGE_PLUS_OFFCUT:
      numPages = 1;
      break;
    case TWO_PAGES:
      numPages = 2;
      break;
  }

  return numPages;
}
}  // anonymous namespace

PageLayout PageLayoutEstimator::estimatePageLayout(const LayoutType layoutType,
                                                   const QImage& input,
                                                   const ImageTransformation& preXform,
                                                   const BinaryThreshold bwThreshold,
                                                   DebugImages* const dbg) {
  if (layoutType == SINGLE_PAGE_UNCUT) {
    return PageLayout(preXform.resultingRect());
  }

  std::unique_ptr<PageLayout> layout(tryCutAtFoldingLine(layoutType, input, preXform, dbg));
  if (layout) {
    return *layout;
  }

  return cutAtWhitespace(layoutType, input, preXform, bwThreshold, dbg);
}

namespace {
class BadTwoPageSplitter {
 public:
  explicit BadTwoPageSplitter(double imageWidth)
      : m_imageCenter(0.5 * imageWidth), m_distFromCenterThreshold(0.6 * m_imageCenter) {}

  /**
   * Returns true if the line is too close to an edge
   * to be the line splitting two pages
   */
  bool operator()(const QLineF& line) {
    const double dist = std::fabs(lineCenterX(line) - m_imageCenter);

    return dist > m_distFromCenterThreshold;
  }

 private:
  double m_imageCenter;
  double m_distFromCenterThreshold;
};
}  // anonymous namespace

/**
 * \brief Attempts to find the folding line and cut the image there.
 *
 * \param layoutType The type of a layout to detect.  If set to
 *        something other than AUTO_LAYOUT_TYPE, the returned
 *        layout will have the same type.  The layout type of
 *        SINGLE_PAGE_UNCUT is not handled here.
 * \param input The input image.  Will be converted to grayscale unless
 *        it's already grayscale.
 * \param preXform The logical transformation applied to the input image.
 *        The resulting page layout will be in transformed coordinates.
 * \param dbg An optional sink for debugging images.
 * \return The detected page layout, or a null unique_ptr if page layout
 *         could not be detected.
 */
std::unique_ptr<PageLayout> PageLayoutEstimator::tryCutAtFoldingLine(const LayoutType layoutType,
                                                                     const QImage& input,
                                                                     const ImageTransformation& preXform,
                                                                     DebugImages* const dbg) {
  const int numPages = page_split::numPages(layoutType, preXform);

  GrayImage grayDownscaled;
  QTransform outToDownscaled;

  const int maxLines = 8;
  std::vector<QLineF> lines(VertLineFinder::findLines(input, preXform, maxLines, dbg,
                                                      numPages == 1 ? &grayDownscaled : nullptr,
                                                      numPages == 1 ? &outToDownscaled : nullptr));

  std::sort(lines.begin(), lines.end(), CenterComparator());

  const QRectF virtualImageRect(preXform.transform().mapRect(input.rect()));
  const QPointF center(virtualImageRect.center());

  if (numPages == 1) {
    // If all of the lines are close to one of the edges,
    // that means they can't be the edges of a pages,
    // so we take only one of them, the one closest to
    // the center.
    while (lines.size() > 1) {  // just to be able to break from it.
      const QLineF leftLine(lines.front());
      const QLineF rightLine(lines.back());
      const double threshold = 0.3 * center.x();
      double leftDist = center.x() - lineCenterX(leftLine);
      double rightDist = center.x() - lineCenterX(rightLine);
      if ((leftDist < 0) != (rightDist < 0)) {
        // They are from the opposite sides
        // from the center line.
        break;
      }

      leftDist = std::fabs(leftDist);
      rightDist = std::fabs(rightDist);
      if ((leftDist < threshold) || (rightDist < threshold)) {
        // At least one of them is relatively close
        // to the center.
        break;
      }

      lines.clear();
      lines.push_back(leftDist < rightDist ? leftLine : rightLine);
      break;
    }

    return autoDetectSinglePageLayout(layoutType, lines, virtualImageRect, grayDownscaled, outToDownscaled, dbg);
  } else {
    assert(numPages == 2);
    // In two page mode we ignore the lines that are too close
    // to the edge.
    lines.erase(std::remove_if(lines.begin(), lines.end(), BadTwoPageSplitter(virtualImageRect.width())), lines.end());

    return autoDetectTwoPageLayout(lines, virtualImageRect);
  }
}  // PageLayoutEstimator::tryCutAtFoldingLine

/**
 * \brief Attempts to find a suitable whitespace to draw a splitting line through.
 *
 * \param layoutType The type of a layout to detect.  If set to
 *        something other than AUTO_LAYOUT_TYPE, the returned
 *        layout will have the same type.
 * \param input The input image.  Will be converted to grayscale unless
 *        it's already grayscale.
 * \param preXform The logical transformation applied to the input image.
 *        The resulting page layout will be in transformed coordinates.
 * \param bwThreshold The global binarization threshold for the input image.
 * \param dbg An optional sink for debugging images.
 * \return Even if no suitable whitespace was found, this function
 *         will return a PageLayout consistent with the layoutType requested.
 */
PageLayout PageLayoutEstimator::cutAtWhitespace(const LayoutType layoutType,
                                                const QImage& input,
                                                const ImageTransformation& preXform,
                                                const BinaryThreshold bwThreshold,
                                                DebugImages* const dbg) {
  QTransform xform;

  // Convert to B/W and rotate.
  BinaryImage img(to300DpiBinary(input, xform, bwThreshold));
  // Note: here we assume the only transformation applied
  // to the input image is orthogonal rotation.
  img = orthogonalRotation(img, preXform.preRotation().toDegrees());
  if (dbg) {
    dbg->add(img, "bw300");
  }

  img = removeGarbageAnd2xDownscale(img, dbg);
  xform.scale(0.5, 0.5);
  if (dbg) {
    dbg->add(img, "no_garbage");
  }

  // From now on we work with 150 dpi images.

  const bool leftOffcut = checkForLeftOffcut(img);
  const bool rightOffcut = checkForRightOffcut(img);

  SkewFinder skewFinder;
  // We work with 150dpi image, so no further reduction.
  skewFinder.setCoarseReduction(0);
  skewFinder.setFineReduction(0);
  skewFinder.setDesiredAccuracy(0.5);  // fine accuracy is not required.
  const Skew skew(skewFinder.findSkew(img));
  if ((skew.angle() != 0.0) && (skew.confidence() >= Skew::GOOD_CONFIDENCE)) {
    const int w = img.width();
    const int h = img.height();
    const double angleDeg = skew.angle();
    const double tg = std::tan(angleDeg * constants::DEG2RAD);

    const auto margin = (int) std::ceil(std::fabs(0.5 * h * tg));
    const int newWidth = w - margin * 2;
    if (newWidth > 0) {
      hShearInPlace(img, tg, 0.5 * h, WHITE);
      BinaryImage newImg(newWidth, h);
      rasterOp<RopSrc>(newImg, newImg.rect(), img, QPoint(margin, 0));
      img.swap(newImg);
      if (dbg) {
        dbg->add(img, "shear_applied");
      }

      QTransform t1;
      t1.translate(-0.5 * w, -0.5 * h);
      QTransform t2;
      t2.shear(tg, 0.0);
      QTransform t3;
      t3.translate(0.5 * w - margin, 0.5 * h);
      xform = xform * t1 * t2 * t3;
    }
  }

  const int numPages = page_split::numPages(layoutType, preXform);
  const PageLayout layout(cutAtWhitespaceDeskewed150(layoutType, numPages, img, leftOffcut, rightOffcut, dbg));

  PageLayout transformedLayout(layout.transformed(xform.inverted()));
  // We don't want a skewed outline!
  transformedLayout.setUncutOutline(preXform.resultingRect());

  return transformedLayout;
}  // PageLayoutEstimator::cutAtWhitespace

/**
 * \brief Attempts to find a suitable whitespace to draw a splitting line through.
 *
 * \param layoutType The type of a layout to detect.  If set to
 *        something other than AUTO_LAYOUT_TYPE, the returned
 *        layout will have the same type.
 * \param numPages The number of pages (1 or 2) in the layout.
 * \param input The black and white, 150 DPI input image.
 * \param leftOffcut True if there seems to be garbage on the left side.
 * \param rightOffcut True if there seems to be garbage on the right side.
 * \param dbg An optional sink for debugging images.
 * \return A PageLAyout consistent with the layoutType requested.
 */
PageLayout PageLayoutEstimator::cutAtWhitespaceDeskewed150(const LayoutType layoutType,
                                                           const int numPages,
                                                           const BinaryImage& input,
                                                           const bool leftOffcut,
                                                           const bool rightOffcut,
                                                           DebugImages* dbg) {
  const int width = input.width();
  const int height = input.height();

  BinaryImage ccImg(input.size(), WHITE);

  {
    ConnCompEraser ccEraser(input, CONN8);
    ConnComp cc;
    while (!(cc = ccEraser.nextConnComp()).isNull()) {
      if ((cc.width() < 5) || (cc.height() < 5)) {
        continue;
      }
      if ((double) cc.height() / cc.width() > 6) {
        continue;
      }
      ccImg.fill(cc.rect(), BLACK);
    }
  }

  if (dbg) {
    dbg->add(ccImg, "ccImg");
  }

  ContentSpanFinder spanFinder;
  spanFinder.setMinContentWidth(2);
  spanFinder.setMinWhitespaceWidth(8);

  std::deque<Span> spans;
  SlicedHistogram hist(ccImg, SlicedHistogram::COLS);
  spanFinder.find(hist, [&](Span s) { spans.push_back(s); });

  if (dbg) {
    visualizeSpans(*dbg, spans, input, "spans");
  }

  if (numPages == 1) {
    return processContentSpansSinglePage(layoutType, spans, width, height, leftOffcut, rightOffcut);
  } else {
    // This helps if we have 2 pages with one page containing nothing
    // but a small amount of garbage.
    removeInsignificantEdgeSpans(spans);
    if (dbg) {
      visualizeSpans(*dbg, spans, input, "spans_refined");
    }

    return processContentSpansTwoPages(layoutType, spans, width, height);
  }
}  // PageLayoutEstimator::cutAtWhitespaceDeskewed150

imageproc::BinaryImage PageLayoutEstimator::to300DpiBinary(const QImage& img,
                                                           QTransform& xform,
                                                           const BinaryThreshold binaryThreshold) {
  const double xfactor = (300.0 * constants::DPI2DPM) / img.dotsPerMeterX();
  const double yfactor = (300.0 * constants::DPI2DPM) / img.dotsPerMeterY();
  if ((std::fabs(xfactor - 1.0) < 0.1) && (std::fabs(yfactor - 1.0) < 0.1)) {
    return BinaryImage(img, binaryThreshold);
  }

  QTransform scaleXform;
  scaleXform.scale(xfactor, yfactor);
  xform *= scaleXform;
  const QSize newSize(std::max(1, (int) std::ceil(xfactor * img.width())),
                      std::max(1, (int) std::ceil(yfactor * img.height())));

  const GrayImage newImage(scaleToGray(GrayImage(img), newSize));

  return BinaryImage(newImage, binaryThreshold);
}

BinaryImage PageLayoutEstimator::removeGarbageAnd2xDownscale(const BinaryImage& image, DebugImages* dbg) {
  BinaryImage reduced(ReduceThreshold(image)(2));
  if (dbg) {
    dbg->add(reduced, "reduced");
  }
  // Remove anything not connected to a bar of at least 4 pixels long.
  BinaryImage nonGarbageSeed(openBrick(reduced, QSize(4, 1)));
  BinaryImage nonGarbageSeed2(openBrick(reduced, QSize(1, 4)));
  rasterOp<RopOr<RopSrc, RopDst>>(nonGarbageSeed, nonGarbageSeed2);
  nonGarbageSeed2.release();
  reduced = seedFill(nonGarbageSeed, reduced, CONN8);
  nonGarbageSeed.release();

  if (dbg) {
    dbg->add(reduced, "garbage_removed");
  }

  BinaryImage horSeed(openBrick(reduced, QSize(200, 14), BLACK));
  BinaryImage verSeed(openBrick(reduced, QSize(14, 300), BLACK));

  rasterOp<RopOr<RopSrc, RopDst>>(horSeed, verSeed);
  BinaryImage seed(horSeed.release());
  verSeed.release();
  if (dbg) {
    dbg->add(seed, "shadows_seed");
  }

  BinaryImage dilated(dilateBrick(reduced, QSize(3, 3)));

  BinaryImage shadowsDilated(seedFill(seed, dilated, CONN8));
  dilated.release();
  if (dbg) {
    dbg->add(shadowsDilated, "shadowsDilated");
  }

  rasterOp<RopSubtract<RopDst, RopSrc>>(reduced, shadowsDilated);

  return reduced;
}  // PageLayoutEstimator::removeGarbageAnd2xDownscale

bool PageLayoutEstimator::checkForLeftOffcut(const BinaryImage& image) {
  const int margin = 2;  // Some scanners leave garbage near page borders.
  const int width = 3;
  QRect rect(margin, 0, width, image.height());
  rect.adjust(0, margin, 0, -margin);

  return image.countBlackPixels(rect) != 0;
}

bool PageLayoutEstimator::checkForRightOffcut(const BinaryImage& image) {
  const int margin = 2;  // Some scanners leave garbage near page borders.
  const int width = 3;
  QRect rect(image.width() - margin - width, 0, width, image.height());
  rect.adjust(0, margin, 0, -margin);

  return image.countBlackPixels(rect) != 0;
}

void PageLayoutEstimator::visualizeSpans(DebugImages& dbg,
                                         const std::deque<Span>& spans,
                                         const BinaryImage& image,
                                         const char* label) {
  const int height = image.height();

  QImage spansImg(image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));

  {
    QPainter painter(&spansImg);
    const QBrush brush(QColor(0xff, 0x00, 0x00, 0x50));
    for (const Span& span : spans) {
      const QRect rect(span.begin(), 0, span.width(), height);
      painter.fillRect(rect, brush);
    }
  }
  dbg.add(spansImg, label);
}

void PageLayoutEstimator::removeInsignificantEdgeSpans(std::deque<Span>& spans) {
  if (spans.empty()) {
    return;
  }
  // GapInfo.first: the amount of content preceding this gap.
  // GapInfo.second: the amount of content following this gap.
  using GapInfo = std::pair<int, int>;

  std::vector<GapInfo> gaps(spans.size() - 1);

  int sum = 0;
  for (unsigned i = 0; i < gaps.size(); ++i) {
    sum += spans[i].width();
    gaps[i].first = sum;
  }
  sum = 0;
  for (auto i = static_cast<int>(gaps.size() - 1); i >= 0; --i) {
    sum += spans[i + 1].width();
    gaps[i].second = sum;
  }
  const int total = sum + spans[0].width();

  int mayBeRemoved = total / 15;

  do {
    const Span& first = spans.front();
    const Span& last = spans.back();
    if (&first == &last) {
      break;
    }
    if (first.width() < last.width()) {
      if (first.width() > mayBeRemoved) {
        break;
      }
      mayBeRemoved -= first.width();
      spans.pop_front();
    } else {
      if (last.width() > mayBeRemoved) {
        break;
      }
      mayBeRemoved -= last.width();
      spans.pop_back();
    }
  } while (!spans.empty());
}  // PageLayoutEstimator::removeInsignificantEdgeSpans

PageLayout PageLayoutEstimator::processContentSpansSinglePage(const LayoutType layoutType,
                                                              const std::deque<Span>& spans,
                                                              const int width,
                                                              const int height,
                                                              const bool leftOffcut,
                                                              const bool rightOffcut) {
  assert(layoutType == AUTO_LAYOUT_TYPE || layoutType == PAGE_PLUS_OFFCUT);

  const QRectF virtualImageRect(0, 0, width, height);

  // Just to be able to break from it.
  while (leftOffcut && !rightOffcut && layoutType == AUTO_LAYOUT_TYPE) {
    double x;
    if (spans.empty()) {
      x = 0.0;
    } else if (spans.front().begin() > 0) {
      x = 0.5 * spans.front().begin();
    } else {
      if (spans.front().width() > width / 2) {
        // Probably it's the content span.
        // Maybe we should cut it from the other side.
        break;
      } else if (spans.size() > 1) {
        x = Span(spans[0], spans[1]).center();
      } else {
        x = std::min(spans[0].end() + 20, width);
      }
    }

    const QLineF rightLine(virtualImageRect.topRight(), virtualImageRect.bottomRight());

    return PageLayout(virtualImageRect, vertLine(x), rightLine);
  }

  // Just to be able to break from it.
  while (rightOffcut && !leftOffcut && layoutType == AUTO_LAYOUT_TYPE) {
    double x;
    if (spans.empty()) {
      x = width;
    } else if (spans.back().end() < width) {
      x = Span(spans.back(), width).center();
    } else {
      if (spans.back().width() > width / 2) {
        // Probably it's the content span.
        // Maybe we should cut it from the other side.
        break;
      } else if (spans.size() > 1) {
        x = Span(spans[spans.size() - 2], spans.back()).center();
      } else {
        x = std::max(spans.back().begin() - 20, 0);
      }
    }

    const QLineF leftLine(virtualImageRect.topLeft(), virtualImageRect.bottomLeft());

    return PageLayout(virtualImageRect, leftLine, vertLine(x));
  }

  if (layoutType == PAGE_PLUS_OFFCUT) {
    const QLineF line1(virtualImageRect.topLeft(), virtualImageRect.bottomLeft());
    const QLineF line2(virtualImageRect.topRight(), virtualImageRect.bottomRight());

    return PageLayout(virtualImageRect, line1, line2);
  } else {
    // Returning a SINGLE_PAGE_UNCUT layout.
    return PageLayout(virtualImageRect);
  }
}  // PageLayoutEstimator::processContentSpansSinglePage

PageLayout PageLayoutEstimator::processContentSpansTwoPages(const LayoutType layoutType,
                                                            const std::deque<Span>& spans,
                                                            const int width,
                                                            const int height) {
  assert(layoutType == AUTO_LAYOUT_TYPE || layoutType == TWO_PAGES);

  const QRectF virtualImageRect(0, 0, width, height);

  double x;
  if (spans.empty()) {
    x = 0.5 * width;
  } else if (spans.size() == 1) {
    return processTwoPagesWithSingleSpan(spans.front(), width, height);
  } else {
    // GapInfo.first: the amount of content preceding this gap.
    // GapInfo.second: the amount of content following this gap.
    using GapInfo = std::pair<int, int>;

    std::vector<GapInfo> gaps(spans.size() - 1);
#if 0
            int sum = 0;
            for (unsigned i = 0; i < gaps.size(); ++i) {
                sum += spans[i].width();
                gaps[i].first = sum;
            }
            sum = 0;
            for (int i = gaps.size() - 1; i >= 0; --i) {
                sum += spans[i + 1].width();
                gaps[i].second = sum;
            }
#else
    const int contentBegin = spans.front().begin();
    const int contentEnd = spans.back().end();
    for (unsigned i = 0; i < gaps.size(); ++i) {
      gaps[i].first = spans[i].end() - contentBegin;
      gaps[i].second = contentEnd - spans[i + 1].begin();
    }
#endif

    int bestGap = 0;
    double bestRatio = 0;
    for (unsigned i = 0; i < gaps.size(); ++i) {
      const double min = std::min(gaps[i].first, gaps[i].second);
      const double max = std::max(gaps[i].first, gaps[i].second);
      const double ratio = min / max;
      if (ratio > bestRatio) {
        bestRatio = ratio;
        bestGap = i;
      }
    }

    if (bestRatio < 0.25) {
      // Probably one of the pages is just empty.
      return processTwoPagesWithSingleSpan(Span(contentBegin, contentEnd), width, height);
    }

    const double acceptableRatio = bestRatio * 0.90;

    int widestGap = bestGap;
    int maxWidth = Span(spans[bestGap], spans[bestGap + 1]).width();
    for (int i = bestGap - 1; i >= 0; --i) {
      const double min = std::min(gaps[i].first, gaps[i].second);
      const double max = std::max(gaps[i].first, gaps[i].second);
      const double ratio = min / max;
      if (ratio < acceptableRatio) {
        break;
      }
      const int width = Span(spans[i], spans[i + 1]).width();
      if (width > maxWidth) {
        maxWidth = width;
        widestGap = i;
      }
    }
    for (auto i = static_cast<unsigned int>(bestGap + 1); i < gaps.size(); ++i) {
      const double min = std::min(gaps[i].first, gaps[i].second);
      const double max = std::max(gaps[i].first, gaps[i].second);
      const double ratio = min / max;
      if (ratio < acceptableRatio) {
        break;
      }
      const int width = Span(spans[i], spans[i + 1]).width();
      if (width > maxWidth) {
        maxWidth = width;
        widestGap = i;
      }
    }

    const Span gap(spans[widestGap], spans[widestGap + 1]);
    x = gap.center();
  }

  return PageLayout(virtualImageRect, vertLine(x));
}  // PageLayoutEstimator::processContentSpansTwoPages

PageLayout PageLayoutEstimator::processTwoPagesWithSingleSpan(const Span& span, int width, int height) {
  const QRectF virtualImageRect(0, 0, width, height);

  const double pageCenter = 0.5 * width;
  const double boxCenter = span.center();
  const double boxHalfWidth = 0.5 * span.width();
  const double distanceToPageCenter = std::fabs(pageCenter - boxCenter) - boxHalfWidth;

  double x;

  if (distanceToPageCenter > 15) {
    x = pageCenter;
  } else {
    const Span leftWs(0, span);
    const Span rightWs(span, width);
    if (leftWs.width() > rightWs.width()) {
      x = std::max(0, span.begin() - 15);
    } else {
      x = std::min(width, span.end() + 15);
    }
  }

  return PageLayout(virtualImageRect, vertLine(x));
}

QLineF PageLayoutEstimator::vertLine(double x) {
  return QLineF(x, 0.0, x, 1.0);
}
}  // namespace page_split