// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DetectVertContentBounds.h"
#include <BinaryImage.h>
#include <Constants.h>
#include <QImage>
#include <QPainter>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "DebugImages.h"
#include "VecNT.h"

using namespace imageproc;

namespace dewarping {
namespace {
struct VertRange {
  int top;
  int bottom;

  VertRange() : top(-1), bottom(-1) {}

  VertRange(int t, int b) : top(t), bottom(b) {}

  bool isValid() const { return top != -1; }
};


struct Segment {
  QLine line;
  Vec2d unitVec;
  int vertDist;

  bool distToVertLine(int vertLineX) const {
    return (bool) std::min<int>(std::abs(line.p1().x() - vertLineX), std::abs(line.p2().x() - vertLineX));
  }

  Segment(const QLine& line, const Vec2d& vec, int dist) : line(line), unitVec(vec), vertDist(dist) {}
};


struct RansacModel {
  std::vector<Segment> segments;
  int totalVertDist;  // Sum of individual Segment::vertDist

  RansacModel() : totalVertDist(0) {}

  void add(const Segment& seg) {
    segments.push_back(seg);
    totalVertDist += seg.vertDist;
  }

  bool betterThan(const RansacModel& other) const { return totalVertDist > other.totalVertDist; }

  void swap(RansacModel& other) {
    segments.swap(other.segments);
    std::swap(totalVertDist, other.totalVertDist);
  }
};


class RansacAlgo {
 public:
  explicit RansacAlgo(const std::vector<Segment>& segments);

  void buildAndAssessModel(const Segment& seedSegment);

  RansacModel& bestModel() { return m_bestModel; }

  const RansacModel& bestModel() const { return m_bestModel; }

 private:
  const std::vector<Segment>& m_segments;
  RansacModel m_bestModel;
  double m_cosThreshold;
};


class SequentialColumnProcessor {
 public:
  enum LeftOrRight { LEFT, RIGHT };

  SequentialColumnProcessor(const QSize& pageSize, LeftOrRight leftOrRight);

  void process(int x, const VertRange& range);

  QLineF approximateWithLine(std::vector<Segment>* dbgSegments = nullptr) const;

  QImage visualizeEnvelope(const QImage& background);

 private:
  bool topMidBottomConcave(QPoint top, QPoint mid, QPoint bottom) const;

  static int crossZ(QPoint v1, QPoint v2);

  bool segmentIsTooLong(QPoint p1, QPoint p2) const;

  QLineF interpolateSegments(const std::vector<Segment>& segments) const;

  // Top and bottom points on the leftmost or the rightmost line.
  QPoint m_leadingTop;
  QPoint m_leadingBottom;
  std::deque<QPoint> m_path;  // Top to bottom.
  int m_maxSegmentSqLen;
  int m_leftMinusOneRightOne;
  LeftOrRight m_leftOrRight;
};


RansacAlgo::RansacAlgo(const std::vector<Segment>& segments)
    : m_segments(segments), m_cosThreshold(std::cos(4.0 * constants::DEG2RAD)) {}

void RansacAlgo::buildAndAssessModel(const Segment& seedSegment) {
  RansacModel curModel;
  curModel.add(seedSegment);

  for (const Segment& seg : m_segments) {
    const double cos = seg.unitVec.dot(seedSegment.unitVec);
    if (cos > m_cosThreshold) {
      curModel.add(seg);
    }
  }

  if (curModel.betterThan(m_bestModel)) {
    curModel.swap(m_bestModel);
  }
}

SequentialColumnProcessor::SequentialColumnProcessor(const QSize& pageSize, LeftOrRight leftOrRight)
    : m_leftMinusOneRightOne(leftOrRight == LEFT ? -1 : 1), m_leftOrRight(leftOrRight) {
  const int w = pageSize.width();
  const int h = pageSize.height();
  m_maxSegmentSqLen = (w * w + h * h) / 3;
}

void SequentialColumnProcessor::process(int x, const VertRange& range) {
  if (!range.isValid()) {
    return;
  }

  if (m_path.empty()) {
    m_leadingTop = QPoint(x, range.top);
    m_leadingBottom = QPoint(x, range.bottom);
    m_path.push_front(m_leadingTop);

    if (range.top != range.bottom) {  // We don't want zero length segments in m_path.
      m_path.push_back(m_leadingBottom);
    }

    return;
  }

  if (range.top < m_path.front().y()) {
    // Growing towards the top.
    const QPoint top(x, range.top);
    // Now we decide if we need to trim the path before
    // adding a new element to it to preserve convexity.
    const size_t size = m_path.size();
    size_t midIdx = 0;
    size_t bottomIdx = 1;

    for (; bottomIdx < size; ++midIdx, ++bottomIdx) {
      if (!topMidBottomConcave(top, m_path[midIdx], m_path[bottomIdx])) {
        break;
      }
    }

    // We avoid trimming the path too much.  This helps cases like a heading
    // wider than the rest of the text.
    if (!segmentIsTooLong(top, m_path[midIdx])) {
      m_path.erase(m_path.begin(), m_path.begin() + midIdx);
    }

    m_path.push_front(top);
  }

  if (range.bottom > m_path.back().y()) {
    // Growing towards the bottom.
    const QPoint bottom(x, range.bottom);

    // Now we decide if we need to trim the path before
    // adding a new element to it to preserve convexity.
    auto midIdx = static_cast<int>(m_path.size() - 1);
    int topIdx = midIdx - 1;

    for (; topIdx >= 0; --topIdx, --midIdx) {
      if (!topMidBottomConcave(m_path[topIdx], m_path[midIdx], bottom)) {
        break;
      }
    }
    // We avoid trimming the path too much.  This helps cases like a heading
    // wider than the rest of the text.
    if (!segmentIsTooLong(bottom, m_path[midIdx])) {
      m_path.erase(m_path.begin() + (midIdx + 1), m_path.end());
    }

    m_path.push_back(bottom);
  }
}  // SequentialColumnProcessor::process

bool SequentialColumnProcessor::topMidBottomConcave(QPoint top, QPoint mid, QPoint bottom) const {
  const int crossZ = this->crossZ(mid - top, bottom - mid);

  return crossZ * m_leftMinusOneRightOne < 0;
}

int SequentialColumnProcessor::crossZ(QPoint v1, QPoint v2) {
  return v1.x() * v2.y() - v2.x() * v1.y();
}

bool SequentialColumnProcessor::segmentIsTooLong(const QPoint p1, const QPoint p2) const {
  const QPoint v(p2 - p1);
  const int sqlen = v.x() * v.x() + v.y() * v.y();

  return sqlen > m_maxSegmentSqLen;
}

QLineF SequentialColumnProcessor::approximateWithLine(std::vector<Segment>* dbgSegments) const {
  using namespace boost::lambda;

  const size_t numPoints = m_path.size();

  std::vector<Segment> segments;
  segments.reserve(numPoints);
  // Collect line segments from m_path and convert them to unit vectors.
  for (size_t i = 1; i < numPoints; ++i) {
    const QPoint pt1(m_path[i - 1]);
    const QPoint pt2(m_path[i]);
    assert(pt2.y() > pt1.y());

    Vec2d vec(pt2 - pt1);
    if (std::fabs(vec[0]) > std::fabs(vec[1])) {
      // We don't want segments that are more horizontal than vertical.
      continue;
    }

    vec /= std::sqrt(vec.squaredNorm());
    segments.emplace_back(QLine(pt1, pt2), vec, pt2.y() - pt1.y());
  }


  // Run RANSAC on the segments.

  RansacAlgo ransac(segments);
  qsrand(0);  // Repeatablity is important.

  // We want to make sure we do pick a few segments closest
  // to the edge, so let's sort segments appropriately
  // and manually feed the best ones to RANSAC.
  const size_t numBestSegments = std::min<size_t>(6, segments.size());
  std::partial_sort(
      segments.begin(), segments.begin() + numBestSegments, segments.end(),
      bind(&Segment::distToVertLine, _1, m_leadingTop.x()) < bind(&Segment::distToVertLine, _2, m_leadingTop.x()));
  for (size_t i = 0; i < numBestSegments; ++i) {
    ransac.buildAndAssessModel(segments[i]);
  }
  // Continue with random samples.
  const int ransacIterations = segments.empty() ? 0 : 200;
  for (int i = 0; i < ransacIterations; ++i) {
    ransac.buildAndAssessModel(segments[qrand() % segments.size()]);
  }

  if (ransac.bestModel().segments.empty()) {
    return QLineF(m_leadingTop, m_leadingTop + QPointF(0, 1));
  }

  const QLineF line(interpolateSegments(ransac.bestModel().segments));

  if (dbgSegments) {
    // Has to be the last thing we do with best model.
    dbgSegments->swap(ransac.bestModel().segments);
  }

  return line;
}  // SequentialColumnProcessor::approximateWithLine

QLineF SequentialColumnProcessor::interpolateSegments(const std::vector<Segment>& segments) const {
  assert(!segments.empty());

  // First, interpolate the angle of segments.
  Vec2d accumVec;
  double accumWeight = 0;

  for (const Segment& seg : segments) {
    const double weight = std::sqrt(double(seg.vertDist));
    accumVec += weight * seg.unitVec;
    accumWeight += weight;
  }

  assert(accumWeight != 0);
  accumVec /= accumWeight;

  QLineF line(m_path.front(), m_path.front() + accumVec);
  Vec2d normal(-accumVec[1], accumVec[0]);
  if ((m_leftOrRight == RIGHT) != (normal[0] < 0)) {
    normal = -normal;
  }
  // normal now points *inside* the image, towards the other bound.
  // Now find the vertex in m_path through which our line should pass.
  for (const QPoint& pt : m_path) {
    if (normal.dot(pt - line.p1()) < 0) {
      line.setP1(pt);
      line.setP2(line.p1() + accumVec);
    }
  }

  return line;
}

QImage SequentialColumnProcessor::visualizeEnvelope(const QImage& background) {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  QPen pen(QColor(0xff, 0, 0, 180));
  pen.setWidthF(3.0);
  painter.setPen(pen);

  if (!m_path.empty()) {
    const std::vector<QPointF> polyline(m_path.begin(), m_path.end());
    painter.drawPolyline(&polyline[0], static_cast<int>(polyline.size()));
  }

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(Qt::blue));
  painter.setOpacity(0.7);
  QRectF rect(0, 0, 9, 9);

  for (QPoint pt : m_path) {
    rect.moveCenter(pt + QPointF(0.5, 0.5));
    painter.drawEllipse(rect);
  }

  return canvas;
}

QImage visualizeSegments(const QImage& background, const std::vector<Segment>& segments) {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  QPen pen(Qt::red);
  pen.setWidthF(3.0);
  painter.setPen(pen);
  painter.setOpacity(0.7);

  for (const Segment& seg : segments) {
    painter.drawLine(seg.line);
  }

  return canvas;
}

// For every column in the image, store the top-most and bottom-most black pixel.
void calculateVertRanges(const imageproc::BinaryImage& image, std::vector<VertRange>& ranges) {
  const int width = image.width();
  const int height = image.height();
  const uint32_t* imageData = image.data();
  const int imageStride = image.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  ranges.reserve(width);

  for (int x = 0; x < width; ++x) {
    ranges.emplace_back();
    VertRange& range = ranges.back();

    const uint32_t mask = msb >> (x & 31);
    const uint32_t* pWord = imageData + (x >> 5);

    int topY = 0;
    for (; topY < height; ++topY, pWord += imageStride) {
      if (*pWord & mask) {
        range.top = topY;
        break;
      }
    }

    int bottomY = height - 1;
    pWord = imageData + bottomY * imageStride + (x >> 5);
    for (; bottomY >= topY; --bottomY, pWord -= imageStride) {
      if (*pWord & mask) {
        range.bottom = bottomY;
        break;
      }
    }
  }
}  // calculateVertRanges

QLineF extendLine(const QLineF& line, int height) {
  QPointF topIntersection;
  QPointF bottomIntersection;

  const QLineF topLine(QPointF(0, 0), QPointF(1, 0));
  const QLineF bottomLine(QPointF(0, height), QPointF(1, height));

  line.intersect(topLine, &topIntersection);
  line.intersect(bottomLine, &bottomIntersection);

  return QLineF(topIntersection, bottomIntersection);
}
}  // namespace

std::pair<QLineF, QLineF> detectVertContentBounds(const imageproc::BinaryImage& image, DebugImages* dbg) {
  const int width = image.width();
  const int height = image.height();

  std::vector<VertRange> cols;
  calculateVertRanges(image, cols);

  SequentialColumnProcessor leftProcessor(image.size(), SequentialColumnProcessor::LEFT);
  for (int x = 0; x < width; ++x) {
    leftProcessor.process(x, cols[x]);
  }

  SequentialColumnProcessor rightProcessor(image.size(), SequentialColumnProcessor::RIGHT);
  for (int x = width - 1; x >= 0; --x) {
    rightProcessor.process(x, cols[x]);
  }

  if (dbg) {
    const QImage background(image.toQImage().convertToFormat(QImage::Format_RGB32));
    dbg->add(leftProcessor.visualizeEnvelope(background), "left_envelope");
    dbg->add(rightProcessor.visualizeEnvelope(background), "right_envelope");
  }

  std::pair<QLineF, QLineF> bounds;

  std::vector<Segment> segments;
  std::vector<Segment>* dbgSegments = dbg ? &segments : nullptr;

  QLineF leftLine(leftProcessor.approximateWithLine(dbgSegments));
  leftLine.translate(-1, 0);
  bounds.first = extendLine(leftLine, height);
  if (dbg) {
    dbg->add(visualizeSegments(image.toQImage(), *dbgSegments), "left_ransac_model");
  }

  QLineF rightLine(rightProcessor.approximateWithLine(dbgSegments));
  rightLine.translate(1, 0);
  bounds.second = extendLine(rightLine, height);
  if (dbg) {
    dbg->add(visualizeSegments(image.toQImage(), *dbgSegments), "right_ransac_model");
  }

  return bounds;
}  // detectVertContentBounds
}  // namespace dewarping