// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_RASTLINEFINDER_H_
#define SCANTAILOR_IMAGEPROC_RASTLINEFINDER_H_

#include <QLineF>
#include <QPointF>
#include <cstddef>
#include <string>
#include <vector>

#include "PriorityQueue.h"

namespace imageproc {
class RastLineFinderParams {
 public:
  RastLineFinderParams();

  /**
   * The algorithm operates in polar coordinates. One of those coordinates
   * is a signed distance to the origin. By default the origin is at (0, 0),
   * but you can set it explicitly with this call.
   */
  void setOrigin(const QPointF& origin) { m_origin = origin; }

  /** \see setOrigin() */
  const QPointF& origin() const { return m_origin; }

  /**
   * By default, all angles are considered. Keeping in mind that line direction
   * doesn't matter, that gives us the range of [0, 180) degrees.
   * This method allows you to provide a custom range to consider.
   * Cases where minAngleDeg > maxAngleDeg are valid. Consider the difference
   * between [20, 200) and [200, 20). The latter one is equivalent to [200, 380).
   *
   * \note This is not the angle between the line and the X axis!
   *       Instead, you take your origin point (which is customizable)
   *       and draw a perpendicular to your line. This vector,
   *       from origin to line, is what defines the line angle.
   *       In other words, after normalizing it to unit length, its
   *       coordinates will correspond to cosine and sine of your angle.
   */
  void setAngleRangeDeg(double minAngleDeg, double maxAngleDeg) {
    m_minAngleDeg = minAngleDeg;
    m_maxAngleDeg = maxAngleDeg;
  }

  /** \see setAngleRangeDeg() */
  double minAngleDeg() const { return m_minAngleDeg; }

  /** \see setAngleRangeDeg() */
  double maxAngleDeg() const { return m_maxAngleDeg; }

  /**
   * Being a recursive subdivision algorithm, it has to stop refining the angle
   * at some point. Angle tolerance is the maximum acceptable error (in degrees)
   * for the lines returned. By default it's set to 0.1 degrees. Setting it to
   * a higher value will improve performance.
   */
  void setAngleToleranceDeg(double toleranceDeg) { m_angleToleranceDeg = toleranceDeg; }

  /** \see setAngleToleranceDeg() */
  double angleToleranceDeg() const { return m_angleToleranceDeg; }

  /**
   * Sets the maximum distance the point is allowed to be from a line
   * to still be considered a part of it. In reality, this value is
   * a lower bound. The upper bound depends on angle tolerance and
   * will tend to the lower bound as angle tolerance tends to zero.
   *
   * \see setAngleTolerance()
   */
  void setMaxDistFromLine(double dist) { m_maxDistFromLine = dist; }

  /** \see setMaxDistFromLine() */
  double maxDistFromLine() const { return m_maxDistFromLine; }

  /**
   * A support point is a point considered to be a part of a line.
   * By default, lines consisting of 3 or more points are considered.
   * The minimum allowed value is 2, while higher values improve performance.
   *
   * \see setMaxDistFromLine()
   */
  void setMinSupportPoints(unsigned pts) { m_minSupportPoints = pts; }

  /**
   * \see setMinSupportPoints()
   */
  unsigned minSupportPoints() const { return m_minSupportPoints; }

  /**
   * \brief Checks if parameters are valid, optionally providing an error string.
   */
  bool validate(std::string* error = nullptr) const;

 private:
  QPointF m_origin;
  double m_minAngleDeg;
  double m_maxAngleDeg;
  double m_angleToleranceDeg;
  double m_maxDistFromLine;
  unsigned m_minSupportPoints;
};


/**
 * \brief Finds lines in point clouds.
 *
 * This class implements the following algorithm:\n
 * Thomas M. Breuel. Finding Lines under Bounded Error.\n
 * Pattern Recognition, 29(1):167-178, 1996.\n
 * http://infoscience.epfl.ch/record/82286/files/93-11.pdf?version=1
 */
class RastLineFinder {
 private:
  class SearchSpace;

  friend void swap(SearchSpace& o1, SearchSpace& o2) { o1.swap(o2); }

 public:
  /**
   * Construct a line finder from a point cloud and a set of parameters.
   *
   * \throw std::invalid_argument if \p params are invalid.
   * \see RastLineFinderParams::validate()
   */
  RastLineFinder(const std::vector<QPointF>& points, const RastLineFinderParams& params);

  /**
   * Look for the next best line in terms of the number of support points.
   * When a line is found, its support points are removed from the lists of
   * support points of other candidate lines.
   *
   * \param[out] pointIdxs If provided, it will be filled with indices of support
   *             points for this line. The indices index the vector of points
   *             that was passed to RastLineFinder constructor.
   * \return If there are no more lines satisfying the search criteria,
   *         a null (default constructed) QLineF is returned. Otherwise,
   *         a line that goes near its support points is returned.
   *         Such a line is not to be treated as a line segment, that is positions
   *         of its endpoints should not be counted upon. In addition, the
   *         line won't be properly fit to its support points, but merely be
   *         close to an optimal line.
   */
  QLineF findNext(std::vector<unsigned>* pointIdxs = nullptr);

 private:
  class Point {
   public:
    QPointF pt;
    bool available;

    explicit Point(const QPointF& p) : pt(p), available(true) {}
  };


  class PointUnavailablePred {
   public:
    explicit PointUnavailablePred(const std::vector<Point>* points) : m_points(points) {}

    bool operator()(unsigned idx) const { return !(*m_points)[idx].available; }

   private:
    const std::vector<Point>* m_points;
  };


  class SearchSpace {
   public:
    SearchSpace();

    SearchSpace(const RastLineFinder& owner,
                float minDist,
                float maxDist,
                float minAngleRad,
                float maxAngleRad,
                const std::vector<unsigned>& candidateIdxs);

    /**
     * Returns a line that corresponds to the center of this search space.
     * The returned line should be treated as an unbounded line rather than
     * line segment, meaning that exact positions of endpoints can't be
     * counted on.
     */
    QLineF representativeLine(const RastLineFinder& owner) const;

    bool subdivideDist(const RastLineFinder& owner, SearchSpace& subspace1, SearchSpace& subspace2) const;

    bool subdivideAngle(const RastLineFinder& owner, SearchSpace& subspace1, SearchSpace& subspace2) const;

    void pruneUnavailablePoints(PointUnavailablePred pred);

    std::vector<unsigned>& pointIdxs() { return m_pointIdxs; }

    const std::vector<unsigned>& pointIdxs() const { return m_pointIdxs; }

    void swap(SearchSpace& other);

   private:
    float m_minDist;  //
    float m_maxDist;  // These are already extended by max-dist-to-line.
    float m_minAngleRad;
    float m_maxAngleRad;
    std::vector<unsigned> m_pointIdxs;  // Indexes into m_points of the parent object.
  };


  class OrderedSearchSpaces : public PriorityQueue<SearchSpace, OrderedSearchSpaces> {
    friend class PriorityQueue<SearchSpace, OrderedSearchSpaces>;

   private:
    void setIndex(SearchSpace& obj, size_t heapIdx) {}

    bool higherThan(const SearchSpace& lhs, const SearchSpace& rhs) const {
      return lhs.pointIdxs().size() > rhs.pointIdxs().size();
    }
  };


  void pushIfGoodEnough(SearchSpace& ssp);

  void markPointsUnavailable(const std::vector<unsigned>& pointIdxs);

  void pruneUnavailablePoints();

  QPointF m_origin;
  double m_angleToleranceRad;
  double m_maxDistFromLine;
  unsigned m_minSupportPoints;
  std::vector<Point> m_points;
  OrderedSearchSpaces m_orderedSearchSpaces;
  bool m_firstLine;
};
}  // namespace imageproc

#endif  // ifndef SCANTAILOR_IMAGEPROC_RASTLINEFINDER_H_
