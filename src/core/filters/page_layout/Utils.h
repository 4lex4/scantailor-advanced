// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_UTILS_H_
#define SCANTAILOR_PAGE_LAYOUT_UTILS_H_

class QPolygonF;
class QPointF;
class QSizeF;
class QRectF;
class Margins;
class ImageTransformation;
class Dpi;

namespace page_layout {
class Alignment;
class Params;

class Utils {
 public:
  Utils() = delete;

  /**
   * \brief Replace an empty content rectangle with a tiny centered one.
   *
   * If the content rectangle is empty (no content on the page), it
   * creates various problems for us.  So, we replace it with a tiny
   * non-empty rectangle centered in the page's crop area, which
   * is retrieved from the ImageTransformation.
   */
  static QRectF adaptContentRect(const ImageTransformation& xform, const QRectF& contentRect);

  /**
   * \brief Calculates the physical size of a rectangle in a transformed space.
   */
  static QSizeF calcRectSizeMM(const ImageTransformation& xform, const QRectF& rect);

  /**
   * \brief Extend a rectangle transformed into a polygon with margins.
   *
   * The first edge of the polygon is considered to be the top edge, the
   * next one is right, and so on.  The polygon must have 4 or 5 vertices
   * (unclosed vs closed polygon).  It must have 90 degree angles and
   * must not be empty.
   */
  static void extendPolyRectWithMargins(QPolygonF& polyRect, const Margins& margins);

  /**
   * \brief Calculates margins to extend hardSizeMm to aggregateHardSizeMm.
   *
   * \param hardSizeMm Source size in millimeters.
   * \param aggregateHardSizeMm Target size in millimeters.
   * \param alignment Determines how exactly to grow the size.
   * \return Non-negative margins that extend \p hardSizeMm to
   *         \p aggregateHardSizeMm.
   */
  static Margins calcSoftMarginsMM(const QSizeF& hardSizeMm,
                                   const QSizeF& aggregateHardSizeMm,
                                   const Alignment& alignment,
                                   const QRectF& contentRect,
                                   const QSizeF& contentSizeMm,
                                   const QRectF& pageRect);

  static Margins calcMarginsMM(const ImageTransformation& xform, const QRectF& pageRect, const QRectF& contentRect);

  /**
   * \brief Calculates the page rect (content + hard margins + soft margins)
   *
   * \param xform Transformations applied to image.
   * \param contentRectPhys Content rectangle in transformed coordinates.
   * \param params Margins, aligment and other parameters.
   * \param aggregateHardSizeMm Maximum width and height across all pages.
   * \return Page rectangle (as a polygon) in physical image coordinates.
   */
  static QPolygonF calcPageRectPhys(const ImageTransformation& xform,
                                    const QPolygonF& contentRectPhys,
                                    const Params& params,
                                    const QSizeF& aggregateHardSizeMm);

  static QPolygonF shiftToRoundedOrigin(const QPolygonF& poly);

  static Params buildDefaultParams(const Dpi& dpi);

 private:
  static QPointF getRightUnitVector(const QPolygonF& polyRect);

  static QPointF getDownUnitVector(const QPolygonF& polyRect);
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_UTILS_H_
