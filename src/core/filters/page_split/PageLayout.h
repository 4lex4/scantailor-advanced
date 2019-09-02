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

#ifndef PAGELAYOUT_H_
#define PAGELAYOUT_H_

#include <QLineF>
#include <QPolygonF>
#include <QString>
#include "LayoutType.h"
#include "PageId.h"

class QRectF;
class QTransform;
class QDomElement;
class QDomDocument;

namespace page_split {
/**
 * The page layout comprises the following:
 * \li A rectangular outline, possibly affine-transformed.
 * \li Layout type indicator.
 * \li Zero, 1 or 2 cutters.
 *
 * Cutters are lines with *arbitrary endpoints* that have different meaning
 * depending on layout type.  The SINGLE_PAGE_UNCUT layout doesn't have any
 * cutters.  The TWO_PAGES layout has one cutter that splits the outline into
 * two pages.  The SINGLE_PAGE_CUT layout has two cutters that cut the outline
 * from two sides.  They don't have a special identity like being a left or
 * a right cutter.  Swapping them won't change the area they bound, and that
 * area is the only thing we care about.
 */
class PageLayout {
 public:
  enum Type { SINGLE_PAGE_UNCUT, SINGLE_PAGE_CUT, TWO_PAGES };

  /**
   * \brief Constructs a null layout.
   */
  PageLayout();

  /**
   * \brief Constructs a SINGLE_PAGE_UNCUT layout.
   */
  explicit PageLayout(const QRectF& full_rect);

  /**
   * \brief Constructs a SINGLE_PAGE_CUT layout.
   */
  PageLayout(const QRectF& full_rect, const QLineF& cutter1, const QLineF& cutter2);

  /**
   * \brief Constructs a TWO_PAGES layout.
   */
  PageLayout(QRectF full_rect, const QLineF& split_line);

  /**
   * \brief Construct a page layout based on XML data.
   */
  explicit PageLayout(const QDomElement& layout_el);

  bool isNull() const;

  Type type() const;

  /**
   * \brief Sets layout type and ensures the internal state
   *        is consistent with the new type.
   */
  void setType(Type type);

  LayoutType toLayoutType() const;

  const QPolygonF& uncutOutline() const;

  /**
   * We don't provide a method to set a polygon, but only a rectangle
   * because we want to make sure the polygon stored internally corresponds
   * to a rectangle.  For example, we want to be sure vertices 0 and 3
   * comprise the line corresponding to a left edge of a rectangle.
   */
  void setUncutOutline(const QRectF& outline);

  /**
   * \brief Get a cutter line.
   *
   * \param idx Cutter index, from 0 inclusive to numCutters() exclusive.
   * \return The cutter line with *arbitrary* endpoints.
   */
  const QLineF& cutterLine(int idx) const;

  /**
   * \brief Set a cutter line.
   *
   * \param idx Cutter index, from 0 inclusive to numCutters() exclusive.
   * \param cutter The new cutter line with *arbitrary* endpoints.
   */
  void setCutterLine(int idx, const QLineF& cutter);

  /**
   * Unlike cutterLine(), which returns a line segment with arbitrary
   * endpoints, inscribedCutterLine() returns a line segment with endpoints
   * touching the edges of the outline polygon.
   *
   * \param idx Cutter index, from 0 inclusive to numCutters() exclusive.
   * \return The cutter line segment with endpoints touching the outline polygon.
   */
  QLineF inscribedCutterLine(int idx) const;

  /**
   * \brief Return the number of cutters (0, 1 or 2) for the current layout type.
   */
  int numCutters() const;

  /**
   * \brief Get the number of pages (1 or 2) for this layout.
   */
  int numSubPages() const;

  /**
   * \brief For single page layouts, return the outline of that page,
   *        otherwise return QPolygonF().
   */
  QPolygonF singlePageOutline() const;

  /**
   * \brief For two page layouts, return the outline of the left page,
   *        otherwise return QPolygonF().
   */
  QPolygonF leftPageOutline() const;

  /**
   * \brief For two pages layouts, return the outline of the right page,
   *        otherwise return QPolygonF().
   */
  QPolygonF rightPageOutline() const;

  /**
   * \brief Determines and calls the appropriate *PageOutline() method.
   */
  QPolygonF pageOutline(PageId::SubPage page) const;

  /**
   * Returns an affine-transformed version of this layout.
   */
  PageLayout transformed(const QTransform& xform) const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  PageLayout(const QPolygonF& outline, const QLineF& cutter1, const QLineF& cutter2, Type type);

  static Type typeFromString(const QString& str);

  static QString typeToString(Type type);

  static QLineF extendToCover(const QLineF& line, const QPolygonF& poly);

  static void ensureSameDirection(const QLineF& line1, QLineF& line2);

  static void maybeAddIntersectionPoint(QPolygonF& poly, const QLineF& line1, const QLineF& line2);

  /**
   * This polygon always corresponds to a rectangle, unless it's empty.
   * It's vertex 0 corresponds to top-left vertex of a rectangle, and
   * it goes clockwise from there, ending at vertex 3.
   */
  QPolygonF m_uncutOutline;

  /**
   * In case of two page layouts, both cutters refer to the split line,
   * and both are supposed to be equal.
   */
  QLineF m_cutter1;
  QLineF m_cutter2;

  Type m_type;
};
}  // namespace page_split
#endif  // ifndef PAGELAYOUT_H_
