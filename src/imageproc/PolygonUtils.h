// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_POLYGONUTILS_H_
#define IMAGEPROC_POLYGONUTILS_H_

#include <vector>

class QPolygonF;
class QPointF;
class QLineF;

namespace imageproc {
class PolygonUtils {
 public:
  /**
   * \brief Adjust vertices to more round coordinates.
   *
   * This method exists to workaround bugs in QPainterPath and QPolygonF
   * composition operations.  It turns out rounding vertex coordinates
   * solves many of those bugs.  We don't round to integer values, we
   * only make very minor adjustments.
   */
  static QPolygonF round(const QPolygonF& poly);

  /**
   * \brief Test if two polygons are logically equal.
   *
   * By logical equality we mean that the following differences don't matter:
   * \li Direction (clockwise vs counter-clockwise).
   * \li Closed vs unclosed.
   * \li Tiny differences in vertex coordinates.
   *
   * \return true if polygons are logically equal, false otherwise.
   */
  static bool fuzzyCompare(const QPolygonF& poly1, const QPolygonF& poly2);

  static QPolygonF convexHull(std::vector<QPointF> point_cloud);

 private:
  class Before;

  static QPointF roundPoint(const QPointF& p);

  static double roundValue(double val);

  static std::vector<QLineF> extractAndNormalizeEdges(const QPolygonF& poly);

  static void maybeAddNormalizedEdge(std::vector<QLineF>& edges, const QPointF& p1, const QPointF& p2);

  static bool fuzzyCompareImpl(const std::vector<QLineF>& lines1, const std::vector<QLineF>& lines2);

  static bool fuzzyCompareImpl(const QLineF& line1, const QLineF& line2);

  static bool fuzzyCompareImpl(const QPointF& p1, const QPointF& p2);

  static const double ROUNDING_MULTIPLIER;
  static const double ROUNDING_RECIP_MULTIPLIER;
};
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_POLYGONUTILS_H_
