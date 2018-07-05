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

#include "PageLayoutEstimator.h"
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
#include "imageproc/Binarize.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/ConnComp.h"
#include "imageproc/ConnCompEraserExt.h"
#include "imageproc/Connectivity.h"
#include "imageproc/Constants.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/Grayscale.h"
#include "imageproc/Morphology.h"
#include "imageproc/OrthogonalRotation.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/RasterOp.h"
#include "imageproc/ReduceThreshold.h"
#include "imageproc/Scale.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Shear.h"
#include "imageproc/SkewFinder.h"
#include "imageproc/SlicedHistogram.h"
#include "imageproc/Transform.h"

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
 * \param layout_type The requested layout type.  The layout type of
 *        SINGLE_PAGE_UNCUT is not handled here.
 * \param ltr_lines Folding line candidates sorted from left to right.
 * \param image_size The dimentions of the page image.
 * \param hor_shadows A downscaled grayscale image that contains
 *        long enough and not too thin horizontal lines.
 * \param dbg An optional sink for debugging images.
 * \return The page layout detected or a null unique_ptr.
 */
std::unique_ptr<PageLayout> autoDetectSinglePageLayout(const LayoutType layout_type,
                                                       const std::vector<QLineF>& ltr_lines,
                                                       const QRectF& virtual_image_rect,
                                                       const GrayImage& gray_downscaled,
                                                       const QTransform& out_to_downscaled,
                                                       DebugImages* dbg) {
  const double image_center = virtual_image_rect.center().x();

  // A loop just to be able to break from it.
  do {
    // This whole branch (loop) leads to SINGLE_PAGE_UNCUT,
    // which conflicts with PAGE_PLUS_OFFCUT.
    if (layout_type == PAGE_PLUS_OFFCUT) {
      break;
    }
    // If we have a single line close to an edge,
    // Or more than one line, with the first and the last
    // ones close to an edge, that looks more like
    // SINGLE_PAGE_CUT layout.
    if (!ltr_lines.empty()) {
      const QLineF& first_line = ltr_lines.front();
      const double line_center = lineCenterX(first_line);
      if (std::fabs(image_center - line_center) > 0.65 * image_center) {
        break;
      }
    }
    if (ltr_lines.size() > 1) {
      const QLineF& last_line = ltr_lines.back();
      const double line_center = lineCenterX(last_line);
      if (std::fabs(image_center - line_center) > 0.65 * image_center) {
        break;
      }
    }

    // Return a SINGLE_PAGE_UNCUT layout.
    return std::make_unique<PageLayout>(virtual_image_rect);
  } while (false);

  if (ltr_lines.empty()) {
    // Impossible to detect the layout type.
    return nullptr;
  } else if (ltr_lines.size() > 1) {
    return std::make_unique<PageLayout>(virtual_image_rect, ltr_lines.front(), ltr_lines.back());
  } else {
    assert(ltr_lines.size() == 1);
    const QLineF& line = ltr_lines.front();
    const double line_center = lineCenterX(line);
    if (line_center < image_center) {
      const QLineF right_line(virtual_image_rect.topRight(), virtual_image_rect.bottomRight());

      return std::make_unique<PageLayout>(virtual_image_rect, line, right_line);
    } else {
      const QLineF left_line(virtual_image_rect.topLeft(), virtual_image_rect.bottomLeft());

      return std::make_unique<PageLayout>(virtual_image_rect, left_line, line);
    }
  }
}  // autoDetectSinglePageLayout

/**
 * \brief Try to auto-detect a page layout for a two-page configuration.
 *
 * \param ltr_lines Folding line candidates sorted from left to right.
 * \param image_size The dimentions of the page image.
 * \return The page layout detected or a null unique_ptr.
 */
std::unique_ptr<PageLayout> autoDetectTwoPageLayout(const std::vector<QLineF>& ltr_lines,
                                                    const QRectF& virtual_image_rect) {
  if (ltr_lines.empty()) {
    // Impossible to detect the page layout.
    return nullptr;
  } else if (ltr_lines.size() == 1) {
    return std::make_unique<PageLayout>(virtual_image_rect, ltr_lines.front());
  }

  // Find the line closest to the center.
  const double image_center = virtual_image_rect.center().x();
  double min_distance = std::numeric_limits<double>::max();
  const QLineF* best_line = nullptr;
  for (const QLineF& line : ltr_lines) {
    const double line_center = lineCenterX(line);
    const double distance = std::fabs(line_center - image_center);
    if (distance < min_distance) {
      min_distance = distance;
      best_line = &line;
    }
  }

  return std::make_unique<PageLayout>(virtual_image_rect, *best_line);
}

int numPages(const LayoutType layout_type, const ImageTransformation& pre_xform) {
  int num_pages = 0;

  switch (layout_type) {
    case AUTO_LAYOUT_TYPE: {
      const QSize image_size(pre_xform.origRect().size().toSize());
      num_pages = ProjectPages::adviseNumberOfLogicalPages(ImageMetadata(image_size, pre_xform.origDpi()),
                                                           pre_xform.preRotation());
      break;
    }
    case SINGLE_PAGE_UNCUT:
    case PAGE_PLUS_OFFCUT:
      num_pages = 1;
      break;
    case TWO_PAGES:
      num_pages = 2;
      break;
  }

  return num_pages;
}
}  // anonymous namespace

PageLayout PageLayoutEstimator::estimatePageLayout(const LayoutType layout_type,
                                                   const QImage& input,
                                                   const ImageTransformation& pre_xform,
                                                   const BinaryThreshold bw_threshold,
                                                   DebugImages* const dbg) {
  if (layout_type == SINGLE_PAGE_UNCUT) {
    return PageLayout(pre_xform.resultingRect());
  }

  std::unique_ptr<PageLayout> layout(tryCutAtFoldingLine(layout_type, input, pre_xform, dbg));
  if (layout) {
    return *layout;
  }

  return cutAtWhitespace(layout_type, input, pre_xform, bw_threshold, dbg);
}

namespace {
class BadTwoPageSplitter {
 public:
  explicit BadTwoPageSplitter(double image_width)
      : m_imageCenter(0.5 * image_width), m_distFromCenterThreshold(0.6 * m_imageCenter) {}

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
 * \param layout_type The type of a layout to detect.  If set to
 *        something other than AUTO_LAYOUT_TYPE, the returned
 *        layout will have the same type.  The layout type of
 *        SINGLE_PAGE_UNCUT is not handled here.
 * \param input The input image.  Will be converted to grayscale unless
 *        it's already grayscale.
 * \param pre_xform The logical transformation applied to the input image.
 *        The resulting page layout will be in transformed coordinates.
 * \param dbg An optional sink for debugging images.
 * \return The detected page layout, or a null unique_ptr if page layout
 *         could not be detected.
 */
std::unique_ptr<PageLayout> PageLayoutEstimator::tryCutAtFoldingLine(const LayoutType layout_type,
                                                                     const QImage& input,
                                                                     const ImageTransformation& pre_xform,
                                                                     DebugImages* const dbg) {
  const int num_pages = numPages(layout_type, pre_xform);

  GrayImage gray_downscaled;
  QTransform out_to_downscaled;

  const int max_lines = 8;
  std::vector<QLineF> lines(VertLineFinder::findLines(input, pre_xform, max_lines, dbg,
                                                      num_pages == 1 ? &gray_downscaled : nullptr,
                                                      num_pages == 1 ? &out_to_downscaled : nullptr));

  std::sort(lines.begin(), lines.end(), CenterComparator());

  const QRectF virtual_image_rect(pre_xform.transform().mapRect(input.rect()));
  const QPointF center(virtual_image_rect.center());

  if (num_pages == 1) {
    // If all of the lines are close to one of the edges,
    // that means they can't be the edges of a pages,
    // so we take only one of them, the one closest to
    // the center.
    while (lines.size() > 1) {  // just to be able to break from it.
      const QLineF left_line(lines.front());
      const QLineF right_line(lines.back());
      const double threshold = 0.3 * center.x();
      double left_dist = center.x() - lineCenterX(left_line);
      double right_dist = center.x() - lineCenterX(right_line);
      if ((left_dist < 0) != (right_dist < 0)) {
        // They are from the opposite sides
        // from the center line.
        break;
      }

      left_dist = std::fabs(left_dist);
      right_dist = std::fabs(right_dist);
      if ((left_dist < threshold) || (right_dist < threshold)) {
        // At least one of them is relatively close
        // to the center.
        break;
      }

      lines.clear();
      lines.push_back(left_dist < right_dist ? left_line : right_line);
      break;
    }

    return autoDetectSinglePageLayout(layout_type, lines, virtual_image_rect, gray_downscaled, out_to_downscaled, dbg);
  } else {
    assert(num_pages == 2);
    // In two page mode we ignore the lines that are too close
    // to the edge.
    lines.erase(std::remove_if(lines.begin(), lines.end(), BadTwoPageSplitter(virtual_image_rect.width())),
                lines.end());

    return autoDetectTwoPageLayout(lines, virtual_image_rect);
  }
}  // PageLayoutEstimator::tryCutAtFoldingLine

/**
 * \brief Attempts to find a suitable whitespace to draw a splitting line through.
 *
 * \param layout_type The type of a layout to detect.  If set to
 *        something other than AUTO_LAYOUT_TYPE, the returned
 *        layout will have the same type.
 * \param input The input image.  Will be converted to grayscale unless
 *        it's already grayscale.
 * \param pre_xform The logical transformation applied to the input image.
 *        The resulting page layout will be in transformed coordinates.
 * \param bw_threshold The global binarization threshold for the input image.
 * \param dbg An optional sink for debugging images.
 * \return Even if no suitable whitespace was found, this function
 *         will return a PageLayout consistent with the layout_type requested.
 */
PageLayout PageLayoutEstimator::cutAtWhitespace(const LayoutType layout_type,
                                                const QImage& input,
                                                const ImageTransformation& pre_xform,
                                                const BinaryThreshold bw_threshold,
                                                DebugImages* const dbg) {
  QTransform xform;

  // Convert to B/W and rotate.
  BinaryImage img(to300DpiBinary(input, xform, bw_threshold));
  // Note: here we assume the only transformation applied
  // to the input image is orthogonal rotation.
  img = orthogonalRotation(img, pre_xform.preRotation().toDegrees());
  if (dbg) {
    dbg->add(img, "bw300");
  }

  img = removeGarbageAnd2xDownscale(img, dbg);
  xform.scale(0.5, 0.5);
  if (dbg) {
    dbg->add(img, "no_garbage");
  }

  // From now on we work with 150 dpi images.

  const bool left_offcut = checkForLeftOffcut(img);
  const bool right_offcut = checkForRightOffcut(img);

  SkewFinder skew_finder;
  // We work with 150dpi image, so no further reduction.
  skew_finder.setCoarseReduction(0);
  skew_finder.setFineReduction(0);
  skew_finder.setDesiredAccuracy(0.5);  // fine accuracy is not required.
  const Skew skew(skew_finder.findSkew(img));
  if ((skew.angle() != 0.0) && (skew.confidence() >= Skew::GOOD_CONFIDENCE)) {
    const int w = img.width();
    const int h = img.height();
    const double angle_deg = skew.angle();
    const double tg = std::tan(angle_deg * constants::DEG2RAD);

    const auto margin = (int) std::ceil(std::fabs(0.5 * h * tg));
    const int new_width = w - margin * 2;
    if (new_width > 0) {
      hShearInPlace(img, tg, 0.5 * h, WHITE);
      BinaryImage new_img(new_width, h);
      rasterOp<RopSrc>(new_img, new_img.rect(), img, QPoint(margin, 0));
      img.swap(new_img);
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

  const int num_pages = numPages(layout_type, pre_xform);
  const PageLayout layout(cutAtWhitespaceDeskewed150(layout_type, num_pages, img, left_offcut, right_offcut, dbg));

  PageLayout transformed_layout(layout.transformed(xform.inverted()));
  // We don't want a skewed outline!
  transformed_layout.setUncutOutline(pre_xform.resultingRect());

  return transformed_layout;
}  // PageLayoutEstimator::cutAtWhitespace

/**
 * \brief Attempts to find a suitable whitespace to draw a splitting line through.
 *
 * \param layout_type The type of a layout to detect.  If set to
 *        something other than AUTO_LAYOUT_TYPE, the returned
 *        layout will have the same type.
 * \param num_pages The number of pages (1 or 2) in the layout.
 * \param input The black and white, 150 DPI input image.
 * \param left_offcut True if there seems to be garbage on the left side.
 * \param right_offcut True if there seems to be garbage on the right side.
 * \param dbg An optional sink for debugging images.
 * \return A PageLAyout consistent with the layout_type requested.
 */
PageLayout PageLayoutEstimator::cutAtWhitespaceDeskewed150(const LayoutType layout_type,
                                                           const int num_pages,
                                                           const BinaryImage& input,
                                                           const bool left_offcut,
                                                           const bool right_offcut,
                                                           DebugImages* dbg) {
  const int width = input.width();
  const int height = input.height();

  BinaryImage cc_img(input.size(), WHITE);

  {
    ConnCompEraser cc_eraser(input, CONN8);
    ConnComp cc;
    while (!(cc = cc_eraser.nextConnComp()).isNull()) {
      if ((cc.width() < 5) || (cc.height() < 5)) {
        continue;
      }
      if ((double) cc.height() / cc.width() > 6) {
        continue;
      }
      cc_img.fill(cc.rect(), BLACK);
    }
  }

  if (dbg) {
    dbg->add(cc_img, "cc_img");
  }

  ContentSpanFinder span_finder;
  span_finder.setMinContentWidth(2);
  span_finder.setMinWhitespaceWidth(8);

  std::deque<Span> spans;
  SlicedHistogram hist(cc_img, SlicedHistogram::COLS);
  span_finder.find(hist, [&](Span s) { spans.push_back(s); });

  if (dbg) {
    visualizeSpans(*dbg, spans, input, "spans");
  }

  if (num_pages == 1) {
    return processContentSpansSinglePage(layout_type, spans, width, height, left_offcut, right_offcut);
  } else {
    // This helps if we have 2 pages with one page containing nothing
    // but a small amount of garbage.
    removeInsignificantEdgeSpans(spans);
    if (dbg) {
      visualizeSpans(*dbg, spans, input, "spans_refined");
    }

    return processContentSpansTwoPages(layout_type, spans, width, height);
  }
}  // PageLayoutEstimator::cutAtWhitespaceDeskewed150

imageproc::BinaryImage PageLayoutEstimator::to300DpiBinary(const QImage& img,
                                                           QTransform& xform,
                                                           const BinaryThreshold binary_threshold) {
  const double xfactor = (300.0 * constants::DPI2DPM) / img.dotsPerMeterX();
  const double yfactor = (300.0 * constants::DPI2DPM) / img.dotsPerMeterY();
  if ((std::fabs(xfactor - 1.0) < 0.1) && (std::fabs(yfactor - 1.0) < 0.1)) {
    return BinaryImage(img, binary_threshold);
  }

  QTransform scale_xform;
  scale_xform.scale(xfactor, yfactor);
  xform *= scale_xform;
  const QSize new_size(std::max(1, (int) std::ceil(xfactor * img.width())),
                       std::max(1, (int) std::ceil(yfactor * img.height())));

  const GrayImage new_image(scaleToGray(GrayImage(img), new_size));

  return BinaryImage(new_image, binary_threshold);
}

BinaryImage PageLayoutEstimator::removeGarbageAnd2xDownscale(const BinaryImage& image, DebugImages* dbg) {
  BinaryImage reduced(ReduceThreshold(image)(2));
  if (dbg) {
    dbg->add(reduced, "reduced");
  }
  // Remove anything not connected to a bar of at least 4 pixels long.
  BinaryImage non_garbage_seed(openBrick(reduced, QSize(4, 1)));
  BinaryImage non_garbage_seed2(openBrick(reduced, QSize(1, 4)));
  rasterOp<RopOr<RopSrc, RopDst>>(non_garbage_seed, non_garbage_seed2);
  non_garbage_seed2.release();
  reduced = seedFill(non_garbage_seed, reduced, CONN8);
  non_garbage_seed.release();

  if (dbg) {
    dbg->add(reduced, "garbage_removed");
  }

  BinaryImage hor_seed(openBrick(reduced, QSize(200, 14), BLACK));
  BinaryImage ver_seed(openBrick(reduced, QSize(14, 300), BLACK));

  rasterOp<RopOr<RopSrc, RopDst>>(hor_seed, ver_seed);
  BinaryImage seed(hor_seed.release());
  ver_seed.release();
  if (dbg) {
    dbg->add(seed, "shadows_seed");
  }

  BinaryImage dilated(dilateBrick(reduced, QSize(3, 3)));

  BinaryImage shadows_dilated(seedFill(seed, dilated, CONN8));
  dilated.release();
  if (dbg) {
    dbg->add(shadows_dilated, "shadows_dilated");
  }

  rasterOp<RopSubtract<RopDst, RopSrc>>(reduced, shadows_dilated);

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

  QImage spans_img(image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));

  {
    QPainter painter(&spans_img);
    const QBrush brush(QColor(0xff, 0x00, 0x00, 0x50));
    for (const Span& span : spans) {
      const QRect rect(span.begin(), 0, span.width(), height);
      painter.fillRect(rect, brush);
    }
  }
  dbg.add(spans_img, label);
}

void PageLayoutEstimator::removeInsignificantEdgeSpans(std::deque<Span>& spans) {
  if (spans.empty()) {
    return;
  }
  // GapInfo.first: the amount of content preceding this gap.
  // GapInfo.second: the amount of content following this gap.
  typedef std::pair<int, int> GapInfo;

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

  int may_be_removed = total / 15;

  do {
    const Span& first = spans.front();
    const Span& last = spans.back();
    if (&first == &last) {
      break;
    }
    if (first.width() < last.width()) {
      if (first.width() > may_be_removed) {
        break;
      }
      may_be_removed -= first.width();
      spans.pop_front();
    } else {
      if (last.width() > may_be_removed) {
        break;
      }
      may_be_removed -= last.width();
      spans.pop_back();
    }
  } while (!spans.empty());
}  // PageLayoutEstimator::removeInsignificantEdgeSpans

PageLayout PageLayoutEstimator::processContentSpansSinglePage(const LayoutType layout_type,
                                                              const std::deque<Span>& spans,
                                                              const int width,
                                                              const int height,
                                                              const bool left_offcut,
                                                              const bool right_offcut) {
  assert(layout_type == AUTO_LAYOUT_TYPE || layout_type == PAGE_PLUS_OFFCUT);

  const QRectF virtual_image_rect(0, 0, width, height);

  // Just to be able to break from it.
  while (left_offcut && !right_offcut && layout_type == AUTO_LAYOUT_TYPE) {
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

    const QLineF right_line(virtual_image_rect.topRight(), virtual_image_rect.bottomRight());

    return PageLayout(virtual_image_rect, vertLine(x), right_line);
  }

  // Just to be able to break from it.
  while (right_offcut && !left_offcut && layout_type == AUTO_LAYOUT_TYPE) {
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

    const QLineF left_line(virtual_image_rect.topLeft(), virtual_image_rect.bottomLeft());

    return PageLayout(virtual_image_rect, left_line, vertLine(x));
  }

  if (layout_type == PAGE_PLUS_OFFCUT) {
    const QLineF line1(virtual_image_rect.topLeft(), virtual_image_rect.bottomLeft());
    const QLineF line2(virtual_image_rect.topRight(), virtual_image_rect.bottomRight());

    return PageLayout(virtual_image_rect, line1, line2);
  } else {
    // Returning a SINGLE_PAGE_UNCUT layout.
    return PageLayout(virtual_image_rect);
  }
}  // PageLayoutEstimator::processContentSpansSinglePage

PageLayout PageLayoutEstimator::processContentSpansTwoPages(const LayoutType layout_type,
                                                            const std::deque<Span>& spans,
                                                            const int width,
                                                            const int height) {
  assert(layout_type == AUTO_LAYOUT_TYPE || layout_type == TWO_PAGES);

  const QRectF virtual_image_rect(0, 0, width, height);

  double x;
  if (spans.empty()) {
    x = 0.5 * width;
  } else if (spans.size() == 1) {
    return processTwoPagesWithSingleSpan(spans.front(), width, height);
  } else {
    // GapInfo.first: the amount of content preceding this gap.
    // GapInfo.second: the amount of content following this gap.
    typedef std::pair<int, int> GapInfo;

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
    const int content_begin = spans.front().begin();
    const int content_end = spans.back().end();
    for (unsigned i = 0; i < gaps.size(); ++i) {
      gaps[i].first = spans[i].end() - content_begin;
      gaps[i].second = content_end - spans[i + 1].begin();
    }
#endif

    int best_gap = 0;
    double best_ratio = 0;
    for (unsigned i = 0; i < gaps.size(); ++i) {
      const double min = std::min(gaps[i].first, gaps[i].second);
      const double max = std::max(gaps[i].first, gaps[i].second);
      const double ratio = min / max;
      if (ratio > best_ratio) {
        best_ratio = ratio;
        best_gap = i;
      }
    }

    if (best_ratio < 0.25) {
      // Probably one of the pages is just empty.
      return processTwoPagesWithSingleSpan(Span(content_begin, content_end), width, height);
    }

    const double acceptable_ratio = best_ratio * 0.90;

    int widest_gap = best_gap;
    int max_width = Span(spans[best_gap], spans[best_gap + 1]).width();
    for (int i = best_gap - 1; i >= 0; --i) {
      const double min = std::min(gaps[i].first, gaps[i].second);
      const double max = std::max(gaps[i].first, gaps[i].second);
      const double ratio = min / max;
      if (ratio < acceptable_ratio) {
        break;
      }
      const int width = Span(spans[i], spans[i + 1]).width();
      if (width > max_width) {
        max_width = width;
        widest_gap = i;
      }
    }
    for (auto i = static_cast<unsigned int>(best_gap + 1); i < gaps.size(); ++i) {
      const double min = std::min(gaps[i].first, gaps[i].second);
      const double max = std::max(gaps[i].first, gaps[i].second);
      const double ratio = min / max;
      if (ratio < acceptable_ratio) {
        break;
      }
      const int width = Span(spans[i], spans[i + 1]).width();
      if (width > max_width) {
        max_width = width;
        widest_gap = i;
      }
    }

    const Span gap(spans[widest_gap], spans[widest_gap + 1]);
    x = gap.center();
  }

  return PageLayout(virtual_image_rect, vertLine(x));
}  // PageLayoutEstimator::processContentSpansTwoPages

PageLayout PageLayoutEstimator::processTwoPagesWithSingleSpan(const Span& span, int width, int height) {
  const QRectF virtual_image_rect(0, 0, width, height);

  const double page_center = 0.5 * width;
  const double box_center = span.center();
  const double box_half_width = 0.5 * span.width();
  const double distance_to_page_center = std::fabs(page_center - box_center) - box_half_width;

  double x;

  if (distance_to_page_center > 15) {
    x = page_center;
  } else {
    const Span left_ws(0, span);
    const Span right_ws(span, width);
    if (left_ws.width() > right_ws.width()) {
      x = std::max(0, span.begin() - 15);
    } else {
      x = std::min(width, span.end() + 15);
    }
  }

  return PageLayout(virtual_image_rect, vertLine(x));
}

QLineF PageLayoutEstimator::vertLine(double x) {
  return QLineF(x, 0.0, x, 1.0);
}
}  // namespace page_split