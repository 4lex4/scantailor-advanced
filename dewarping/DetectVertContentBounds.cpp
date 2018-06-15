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

#include "DetectVertContentBounds.h"
#include <QImage>
#include <QPainter>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "DebugImages.h"
#include "VecNT.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Constants.h"

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

  bool distToVertLine(int vert_line_x) const {
    return (bool) std::min<int>(std::abs(line.p1().x() - vert_line_x), std::abs(line.p2().x() - vert_line_x));
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

  void buildAndAssessModel(const Segment& seed_segment);

  RansacModel& bestModel() { return m_bestModel; }

  const RansacModel& bestModel() const { return m_bestModel; }

 private:
  const std::vector<Segment>& m_rSegments;
  RansacModel m_bestModel;
  double m_cosThreshold;
};


class SequentialColumnProcessor {
 public:
  enum LeftOrRight { LEFT, RIGHT };

  SequentialColumnProcessor(const QSize& page_size, LeftOrRight left_or_right);

  void process(int x, const VertRange& range);

  QLineF approximateWithLine(std::vector<Segment>* dbg_segments = nullptr) const;

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
    : m_rSegments(segments), m_cosThreshold(std::cos(4.0 * constants::DEG2RAD)) {}

void RansacAlgo::buildAndAssessModel(const Segment& seed_segment) {
  RansacModel cur_model;
  cur_model.add(seed_segment);

  for (const Segment& seg : m_rSegments) {
    const double cos = seg.unitVec.dot(seed_segment.unitVec);
    if (cos > m_cosThreshold) {
      cur_model.add(seg);
    }
  }

  if (cur_model.betterThan(m_bestModel)) {
    cur_model.swap(m_bestModel);
  }
}

SequentialColumnProcessor::SequentialColumnProcessor(const QSize& page_size, LeftOrRight left_or_right)
    : m_leftMinusOneRightOne(left_or_right == LEFT ? -1 : 1), m_leftOrRight(left_or_right) {
  const int w = page_size.width();
  const int h = page_size.height();
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
    size_t mid_idx = 0;
    size_t bottom_idx = 1;

    for (; bottom_idx < size; ++mid_idx, ++bottom_idx) {
      if (!topMidBottomConcave(top, m_path[mid_idx], m_path[bottom_idx])) {
        break;
      }
    }

    // We avoid trimming the path too much.  This helps cases like a heading
    // wider than the rest of the text.
    if (!segmentIsTooLong(top, m_path[mid_idx])) {
      m_path.erase(m_path.begin(), m_path.begin() + mid_idx);
    }

    m_path.push_front(top);
  }

  if (range.bottom > m_path.back().y()) {
    // Growing towards the bottom.
    const QPoint bottom(x, range.bottom);

    // Now we decide if we need to trim the path before
    // adding a new element to it to preserve convexity.
    auto mid_idx = static_cast<int>(m_path.size() - 1);
    int top_idx = mid_idx - 1;

    for (; top_idx >= 0; --top_idx, --mid_idx) {
      if (!topMidBottomConcave(m_path[top_idx], m_path[mid_idx], bottom)) {
        break;
      }
    }
    // We avoid trimming the path too much.  This helps cases like a heading
    // wider than the rest of the text.
    if (!segmentIsTooLong(bottom, m_path[mid_idx])) {
      m_path.erase(m_path.begin() + (mid_idx + 1), m_path.end());
    }

    m_path.push_back(bottom);
  }
}  // SequentialColumnProcessor::process

bool SequentialColumnProcessor::topMidBottomConcave(QPoint top, QPoint mid, QPoint bottom) const {
  const int cross_z = crossZ(mid - top, bottom - mid);

  return cross_z * m_leftMinusOneRightOne < 0;
}

int SequentialColumnProcessor::crossZ(QPoint v1, QPoint v2) {
  return v1.x() * v2.y() - v2.x() * v1.y();
}

bool SequentialColumnProcessor::segmentIsTooLong(const QPoint p1, const QPoint p2) const {
  const QPoint v(p2 - p1);
  const int sqlen = v.x() * v.x() + v.y() * v.y();

  return sqlen > m_maxSegmentSqLen;
}

QLineF SequentialColumnProcessor::approximateWithLine(std::vector<Segment>* dbg_segments) const {
  using namespace boost::lambda;

  const size_t num_points = m_path.size();

  std::vector<Segment> segments;
  segments.reserve(num_points);
  // Collect line segments from m_path and convert them to unit vectors.
  for (size_t i = 1; i < num_points; ++i) {
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
  const size_t num_best_segments = std::min<size_t>(6, segments.size());
  std::partial_sort(
      segments.begin(), segments.begin() + num_best_segments, segments.end(),
      bind(&Segment::distToVertLine, _1, m_leadingTop.x()) < bind(&Segment::distToVertLine, _2, m_leadingTop.x()));
  for (size_t i = 0; i < num_best_segments; ++i) {
    ransac.buildAndAssessModel(segments[i]);
  }
  // Continue with random samples.
  const int ransac_iterations = segments.empty() ? 0 : 200;
  for (int i = 0; i < ransac_iterations; ++i) {
    ransac.buildAndAssessModel(segments[qrand() % segments.size()]);
  }

  if (ransac.bestModel().segments.empty()) {
    return QLineF(m_leadingTop, m_leadingTop + QPointF(0, 1));
  }

  const QLineF line(interpolateSegments(ransac.bestModel().segments));

  if (dbg_segments) {
    // Has to be the last thing we do with best model.
    dbg_segments->swap(ransac.bestModel().segments);
  }

  return line;
}  // SequentialColumnProcessor::approximateWithLine

QLineF SequentialColumnProcessor::interpolateSegments(const std::vector<Segment>& segments) const {
  assert(!segments.empty());

  // First, interpolate the angle of segments.
  Vec2d accum_vec;
  double accum_weight = 0;

  for (const Segment& seg : segments) {
    const double weight = std::sqrt(double(seg.vertDist));
    accum_vec += weight * seg.unitVec;
    accum_weight += weight;
  }

  assert(accum_weight != 0);
  accum_vec /= accum_weight;

  QLineF line(m_path.front(), m_path.front() + accum_vec);
  Vec2d normal(-accum_vec[1], accum_vec[0]);
  if ((m_leftOrRight == RIGHT) != (normal[0] < 0)) {
    normal = -normal;
  }
  // normal now points *inside* the image, towards the other bound.
  // Now find the vertex in m_path through which our line should pass.
  for (const QPoint& pt : m_path) {
    if (normal.dot(pt - line.p1()) < 0) {
      line.setP1(pt);
      line.setP2(line.p1() + accum_vec);
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
  const uint32_t* image_data = image.data();
  const int image_stride = image.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  ranges.reserve(width);

  for (int x = 0; x < width; ++x) {
    ranges.emplace_back();
    VertRange& range = ranges.back();

    const uint32_t mask = msb >> (x & 31);
    const uint32_t* p_word = image_data + (x >> 5);

    int top_y = 0;
    for (; top_y < height; ++top_y, p_word += image_stride) {
      if (*p_word & mask) {
        range.top = top_y;
        break;
      }
    }

    int bottom_y = height - 1;
    p_word = image_data + bottom_y * image_stride + (x >> 5);
    for (; bottom_y >= top_y; --bottom_y, p_word -= image_stride) {
      if (*p_word & mask) {
        range.bottom = bottom_y;
        break;
      }
    }
  }
}  // calculateVertRanges

QLineF extendLine(const QLineF& line, int height) {
  QPointF top_intersection;
  QPointF bottom_intersection;

  const QLineF top_line(QPointF(0, 0), QPointF(1, 0));
  const QLineF bottom_line(QPointF(0, height), QPointF(1, height));

  line.intersect(top_line, &top_intersection);
  line.intersect(bottom_line, &bottom_intersection);

  return QLineF(top_intersection, bottom_intersection);
}
}  // namespace

std::pair<QLineF, QLineF> detectVertContentBounds(const imageproc::BinaryImage& image, DebugImages* dbg) {
  const int width = image.width();
  const int height = image.height();

  std::vector<VertRange> cols;
  calculateVertRanges(image, cols);

  SequentialColumnProcessor left_processor(image.size(), SequentialColumnProcessor::LEFT);
  for (int x = 0; x < width; ++x) {
    left_processor.process(x, cols[x]);
  }

  SequentialColumnProcessor right_processor(image.size(), SequentialColumnProcessor::RIGHT);
  for (int x = width - 1; x >= 0; --x) {
    right_processor.process(x, cols[x]);
  }

  if (dbg) {
    const QImage background(image.toQImage().convertToFormat(QImage::Format_RGB32));
    dbg->add(left_processor.visualizeEnvelope(background), "left_envelope");
    dbg->add(right_processor.visualizeEnvelope(background), "right_envelope");
  }

  std::pair<QLineF, QLineF> bounds;

  std::vector<Segment> segments;
  std::vector<Segment>* dbg_segments = dbg ? &segments : nullptr;

  QLineF left_line(left_processor.approximateWithLine(dbg_segments));
  left_line.translate(-1, 0);
  bounds.first = extendLine(left_line, height);
  if (dbg) {
    dbg->add(visualizeSegments(image.toQImage(), *dbg_segments), "left_ransac_model");
  }

  QLineF right_line(right_processor.approximateWithLine(dbg_segments));
  right_line.translate(1, 0);
  bounds.second = extendLine(right_line, height);
  if (dbg) {
    dbg->add(visualizeSegments(image.toQImage(), *dbg_segments), "right_ransac_model");
  }

  return bounds;
}  // detectVertContentBounds
}  // namespace dewarping