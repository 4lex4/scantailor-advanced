// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_PAGELAYOUTESTIMATOR_H_
#define SCANTAILOR_PAGE_SPLIT_PAGELAYOUTESTIMATOR_H_

#include <VirtualFunction.h>

#include <QLineF>
#include <deque>
#include <memory>

#include "LayoutType.h"

class QRect;
class QPoint;
class QImage;
class QTransform;
class ImageTransformation;
class DebugImages;
class Span;

namespace imageproc {
class BinaryImage;
class BinaryThreshold;
}  // namespace imageproc

namespace page_split {
class PageLayout;

class PageLayoutEstimator {
 public:
  /**
   * \brief Estimates the page layout according to the provided layout type.
   *
   * \param layoutType The type of a layout to detect.  If set to
   *        something other than Rule::AUTO_DETECT, the returned
   *        layout will have the same type.
   * \param input The input image.  Will be converted to grayscale unless
   *        it's already grayscale.
   * \param preXform The logical transformation applied to the input image.
   *        The resulting page layout will be in transformed coordinates.
   * \param bwThreshold The global binarization threshold for the
   *        input image.
   * \param dbg An optional sink for debugging images.
   * \return The estimated PageLayout of type consistent with the
   *         requested layout type.
   */
  static PageLayout estimatePageLayout(LayoutType layoutType,
                                       const QImage& input,
                                       const ImageTransformation& preXform,
                                       imageproc::BinaryThreshold bwThreshold,
                                       DebugImages* dbg = nullptr);

 private:
  static std::unique_ptr<PageLayout> tryCutAtFoldingLine(LayoutType layoutType,
                                                         const QImage& input,
                                                         const ImageTransformation& preXform,
                                                         DebugImages* dbg);

  static PageLayout cutAtWhitespace(LayoutType layoutType,
                                    const QImage& input,
                                    const ImageTransformation& preXform,
                                    imageproc::BinaryThreshold bwThreshold,
                                    DebugImages* dbg);

  static PageLayout cutAtWhitespaceDeskewed150(LayoutType layoutType,
                                               int numPages,
                                               const imageproc::BinaryImage& input,
                                               bool leftOffcut,
                                               bool rightOffcut,
                                               DebugImages* dbg);

  static imageproc::BinaryImage to300DpiBinary(const QImage& img,
                                               QTransform& xform,
                                               imageproc::BinaryThreshold threshold);

  static imageproc::BinaryImage removeGarbageAnd2xDownscale(const imageproc::BinaryImage& image, DebugImages* dbg);

  static bool checkForLeftOffcut(const imageproc::BinaryImage& image);

  static bool checkForRightOffcut(const imageproc::BinaryImage& image);

  static void visualizeSpans(DebugImages& dbg,
                             const std::deque<Span>& spans,
                             const imageproc::BinaryImage& image,
                             const char* label);

  static void removeInsignificantEdgeSpans(std::deque<Span>& spans);

  static PageLayout processContentSpansSinglePage(LayoutType layoutType,
                                                  const std::deque<Span>& spans,
                                                  int width,
                                                  int height,
                                                  bool leftOffcut,
                                                  bool rightOffcut);

  static PageLayout processContentSpansTwoPages(LayoutType layoutType,
                                                const std::deque<Span>& spans,
                                                int width,
                                                int height);

  static PageLayout processTwoPagesWithSingleSpan(const Span& span, int width, int height);

  static QLineF vertLine(double x);
};
}  // namespace page_split
#endif  // ifndef SCANTAILOR_PAGE_SPLIT_PAGELAYOUTESTIMATOR_H_
