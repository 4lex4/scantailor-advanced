// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_HOUGHLINEDETECTOR_H_
#define SCANTAILOR_IMAGEPROC_HOUGHLINEDETECTOR_H_

#include <QPointF>
#include <vector>

class QSize;
class QLineF;
class QImage;

namespace imageproc {
class BinaryImage;

/**
 * \brief A line detected by HoughLineDetector.
 *
 * A line is represented by a unit-size vector perpendicular to
 * the line, and a distance along that vector to a point on the line.
 * In other words, unit_vector * distance is a point on the line and
 * unit_vector is the normal vector for that line.
 */
class HoughLine {
 public:
  HoughLine() : m_normUnitVector(), m_distance(), m_quality() {}

  HoughLine(const QPointF& normUv, double distance, unsigned quality)
      : m_normUnitVector(normUv), m_distance(distance), m_quality(quality) {}

  const QPointF& normUnitVector() const { return m_normUnitVector; }

  double distance() const { return m_distance; }

  /**
   * \brief The sum of weights of points on the line.
   *
   * The weight of a point is an argument to HoughLineDetector::put().
   */
  unsigned quality() const { return m_quality; }

  QPointF pointAtY(double y) const;

  QPointF pointAtX(double x) const;

  /**
   * \brief Returns an arbitrary line segment of length 1.
   */
  QLineF unitSegment() const;

 private:
  QPointF m_normUnitVector;
  double m_distance;
  unsigned m_quality;
};


class HoughLineDetector {
 public:
  /**
   * \brief A line finder based on Hough transform.
   *
   * \param inputDimensions The range of valid input coordinates,
   *        which are [0, width - 1] for x and [0, height - 1] for y.
   * \param distanceResolution The distance in input units that
   *        represents the width of the lines we are searching for.
   *        The more this parameter is, the more pixels on the sides
   *        of a line will be considered a part of it.
   *        Normally this parameter greater than 1, but theoretically
   *        it maybe any positive value.
   * \param startAngle The first angle to check for.  This angle
   *        is between the normal vector of a line we are looking for
   *        and the X axis.  The angle is in degrees.
   * \param angleDelta The difference (in degrees) between an
   *        angle and the next one.
   * \param numAngles The number of angles to check.
   */
  HoughLineDetector(const QSize& inputDimensions,
                    double distanceResolution,
                    double startAngle,
                    double angleDelta,
                    int numAngles);

  /**
   * \brief Processes a point with a specified weight.
   */
  void process(int x, int y, unsigned weight = 1);

  QImage visualizeHoughSpace(unsigned lowerBound) const;

  /**
   * \brief Returns the lines found among the input points.
   *
   * The lines will be ordered by the descending quality.
   * \see HoughLineDetector::Line::quality()
   *
   * \param qualityLowerBound The minimum acceptable line quality.
   */
  std::vector<HoughLine> findLines(unsigned qualityLowerBound) const;

 private:
  class GreaterQualityFirst;

  static BinaryImage findHistogramPeaks(const std::vector<unsigned>& hist, int width, int height, unsigned lowerBound);

  static BinaryImage findPeakCandidates(const std::vector<unsigned>& hist, int width, int height, unsigned lowerBound);

  static void incrementBinsMasked(std::vector<unsigned>& hist, int width, int height, const BinaryImage& mask);

  static void max5x5(const std::vector<unsigned>& src, std::vector<unsigned>& dst, int width, int height);

  static void max3x1(const std::vector<unsigned>& src, std::vector<unsigned>& dst, int width, int height);

  static void max1x3(const std::vector<unsigned>& src, std::vector<unsigned>& dst, int width, int height);

  static BinaryImage buildEqualMap(const std::vector<unsigned>& src1,
                                   const std::vector<unsigned>& src2,
                                   int width,
                                   int height,
                                   unsigned lowerBound);

  /**
   * \brief A 2D histogram laid out in raster order.
   *
   * Rows correspond to line angles while columns correspond to
   * line distances from the origin.
   */
  std::vector<unsigned> m_histogram;

  /**
   * \brief An array of sines (y) and cosines(x) of angles we working with.
   */
  std::vector<QPointF> m_angleUnitVectors;

  /**
   * \see HoughLineDetector:HoughLineDetector()
   */
  double m_distanceResolution;

  /**
   * 1.0 / m_distanceResolution
   */
  double m_recipDistanceResolution;

  /**
   * The value to be added to distance to make sure it's positive.
   */
  double m_distanceBias;

  /**
   * The width of m_histogram.
   */
  int m_histWidth;

  /**
   * The height of m_histogram.
   */
  int m_histHeight;
};
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_HOUGHLINEDETECTOR_H_
