/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "PolygonRasterizer.h"
#include <QImage>
#include <QPainterPath>
#include <QPolygonF>
#include <boost/foreach.hpp>
#include <cassert>
#include "BinaryImage.h"
#include "PolygonUtils.h"

namespace imageproc {
/**
 * \brief A non-horizontal and non zero-length polygon edge.
 */
class PolygonRasterizer::Edge {
 public:
  Edge(const QPointF& top, const QPointF& bottom, int vert_direction);

  Edge(const QPointF& from, const QPointF& to);

  const QPointF& top() const { return m_top; }

  const QPointF& bottom() const { return m_bottom; }

  double topY() const { return m_top.y(); }

  double bottomY() const { return m_bottom.y(); }

  double xForY(double y) const;

  int vertDirection() const { return m_vertDirection; }

 private:
  QPointF m_top;
  QPointF m_bottom;
  double m_deltaX;
  double m_reDeltaY;
  int m_vertDirection;  // 1: down, -1: up
};


/**
 * \brief A non-overlaping edge component.
 */
class PolygonRasterizer::EdgeComponent {
 public:
  EdgeComponent(const Edge* edge, double top, double bottom) : m_top(top), m_bottom(bottom), m_x(), m_pEdge(edge) {}

  double top() const { return m_top; }

  double bottom() const { return m_bottom; }

  const Edge& edge() const { return *m_pEdge; }

  double x() const { return m_x; }

  void setX(double x) { m_x = x; }

 private:
  double m_top;
  double m_bottom;
  double m_x;
  const Edge* m_pEdge;
};


class PolygonRasterizer::EdgeOrderY {
 public:
  bool operator()(const EdgeComponent& lhs, const EdgeComponent& rhs) const { return lhs.top() < rhs.top(); }

  bool operator()(const EdgeComponent& lhs, double rhs) const {
    return lhs.bottom() <= rhs;  // bottom is not a part of the interval.
  }

  bool operator()(double lhs, const EdgeComponent& rhs) const { return lhs < rhs.top(); }
};


class PolygonRasterizer::EdgeOrderX {
 public:
  bool operator()(const EdgeComponent& lhs, const EdgeComponent& rhs) const { return lhs.x() < rhs.x(); }
};


class PolygonRasterizer::Rasterizer {
 public:
  Rasterizer(const QRect& image_rect, const QPolygonF& poly, Qt::FillRule fill_rule, bool invert);

  void fillBinary(BinaryImage& image, BWColor color) const;

  void fillGrayscale(QImage& image, uint8_t color) const;

 private:
  void prepareEdges();

  static void oddEvenLineBinary(const EdgeComponent* edges, int num_edges, uint32_t* line, uint32_t pattern);

  static void oddEvenLineGrayscale(const EdgeComponent* edges, int num_edges, uint8_t* line, uint8_t color);

  static void windingLineBinary(const EdgeComponent* edges,
                                int num_edges,
                                uint32_t* line,
                                uint32_t pattern,
                                bool invert);

  static void windingLineGrayscale(const EdgeComponent* edges,
                                   int num_edges,
                                   uint8_t* line,
                                   uint8_t color,
                                   bool invert);

  static void fillBinarySegment(int x_from, int x_to, uint32_t* line, uint32_t pattern);

  std::vector<Edge> m_edges;  // m_edgeComponents references m_edges.
  std::vector<EdgeComponent> m_edgeComponents;
  QRect m_imageRect;
  QPolygonF m_fillPoly;
  QRectF m_boundingBox;
  Qt::FillRule m_fillRule;
  bool m_invert;
};


/*============================= PolygonRasterizer ===========================*/

void PolygonRasterizer::fill(BinaryImage& image,
                             const BWColor color,
                             const QPolygonF& poly,
                             const Qt::FillRule fill_rule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }

  Rasterizer rasterizer(image.rect(), poly, fill_rule, false);
  rasterizer.fillBinary(image, color);
}

void PolygonRasterizer::fillExcept(BinaryImage& image,
                                   const BWColor color,
                                   const QPolygonF& poly,
                                   const Qt::FillRule fill_rule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }

  Rasterizer rasterizer(image.rect(), poly, fill_rule, true);
  rasterizer.fillBinary(image, color);
}

void PolygonRasterizer::grayFill(QImage& image,
                                 const unsigned char color,
                                 const QPolygonF& poly,
                                 const Qt::FillRule fill_rule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }
  if ((image.format() != QImage::Format_Indexed8) || !image.isGrayscale()) {
    throw std::invalid_argument("PolygonRasterizer: target image is not grayscale");
  }

  Rasterizer rasterizer(image.rect(), poly, fill_rule, false);
  rasterizer.fillGrayscale(image, color);
}

void PolygonRasterizer::grayFillExcept(QImage& image,
                                       const unsigned char color,
                                       const QPolygonF& poly,
                                       const Qt::FillRule fill_rule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }
  if ((image.format() != QImage::Format_Indexed8) || !image.isGrayscale()) {
    throw std::invalid_argument("PolygonRasterizer: target image is not grayscale");
  }

  Rasterizer rasterizer(image.rect(), poly, fill_rule, true);
  rasterizer.fillGrayscale(image, color);
}

/*======================= PolygonRasterizer::Edge ==========================*/

PolygonRasterizer::Edge::Edge(const QPointF& top, const QPointF& bottom, const int vert_direction)
    : m_top(top),
      m_bottom(bottom),
      m_deltaX(bottom.x() - top.x()),
      m_reDeltaY(1.0 / (bottom.y() - top.y())),
      m_vertDirection(vert_direction) {}

PolygonRasterizer::Edge::Edge(const QPointF& from, const QPointF& to) {
  if (from.y() < to.y()) {
    m_vertDirection = 1;
    m_top = from;
    m_bottom = to;
  } else {
    m_vertDirection = -1;
    m_top = to;
    m_bottom = from;
  }
  m_deltaX = m_bottom.x() - m_top.x();
  m_reDeltaY = 1.0 / (m_bottom.y() - m_top.y());
}

double PolygonRasterizer::Edge::xForY(double y) const {
  const double fraction = (y - m_top.y()) * m_reDeltaY;

  return m_top.x() + m_deltaX * fraction;
}

/*=================== PolygonRasterizer::Rasterizer ====================*/

PolygonRasterizer::Rasterizer::Rasterizer(const QRect& image_rect,
                                          const QPolygonF& poly,
                                          const Qt::FillRule fill_rule,
                                          const bool invert)
    : m_imageRect(image_rect), m_fillRule(fill_rule), m_invert(invert) {
  QPainterPath path1;
  path1.setFillRule(fill_rule);
  path1.addRect(image_rect);

  QPainterPath path2;
  path2.setFillRule(fill_rule);
  path2.addPolygon(PolygonUtils::round(poly));
  path2.closeSubpath();

  m_fillPoly = path1.intersected(path2).toFillPolygon();

  if (invert) {
    m_boundingBox = path1.subtracted(path2).boundingRect();
  } else {
    m_boundingBox = m_fillPoly.boundingRect();
  }

  prepareEdges();
}

void PolygonRasterizer::Rasterizer::prepareEdges() {
  const int num_verts = m_fillPoly.size();
  if (num_verts == 0) {
    return;
  }

  // Collect the edges, excluding horizontal and null ones.
  m_edges.reserve(num_verts + 2);
  for (int i = 0; i < num_verts - 1; ++i) {
    const QPointF from(m_fillPoly[i]);
    const QPointF to(m_fillPoly[i + 1]);
    if (from.y() != to.y()) {
      m_edges.emplace_back(from, to);
    }
  }

  assert(m_fillPoly.isClosed());

  if (m_invert) {
    // Add left and right edges with neutral direction (0),
    // to avoid confusing a winding fill.
    const QRectF rect(m_imageRect);
    m_edges.emplace_back(rect.topLeft(), rect.bottomLeft(), 0);
    m_edges.emplace_back(rect.topRight(), rect.bottomRight(), 0);
  }

  // Create an ordered list of y coordinates of polygon vertexes.
  std::vector<double> y_values;
  y_values.reserve(num_verts + 2);
  for (const QPointF& pt : m_fillPoly) {
    y_values.push_back(pt.y());
  }

  if (m_invert) {
    y_values.push_back(0.0);
    y_values.push_back(m_imageRect.height());
  }

  // Sort and remove duplicates.
  std::sort(y_values.begin(), y_values.end());
  y_values.erase(std::unique(y_values.begin(), y_values.end()), y_values.end());

  // Break edges into non-overlaping components, then sort them.
  m_edgeComponents.reserve(m_edges.size());
  for (const Edge& edge : m_edges) {
    auto it(std::lower_bound(y_values.begin(), y_values.end(), edge.topY()));

    assert(*it == edge.topY());

    do {
      auto next(it);
      ++next;
      assert(next != y_values.end());
      m_edgeComponents.emplace_back(&edge, *it, *next);
      it = next;
    } while (*it != edge.bottomY());
  }

  std::sort(m_edgeComponents.begin(), m_edgeComponents.end(), EdgeOrderY());
}  // PolygonRasterizer::Rasterizer::prepareEdges

void PolygonRasterizer::Rasterizer::fillBinary(BinaryImage& image, const BWColor color) const {
  std::vector<EdgeComponent> edges_for_line;
  typedef std::vector<EdgeComponent>::const_iterator EdgeIter;

  uint32_t* line = image.data();
  const int wpl = image.wordsPerLine();
  const uint32_t pattern = (color == WHITE) ? 0 : ~uint32_t(0);

  int i = qRound(m_boundingBox.top());
  line += i * wpl;
  const int limit = qRound(m_boundingBox.bottom());
  for (; i < limit; ++i, line += wpl, edges_for_line.clear()) {
    const double y = i + 0.5;

    // Get edges intersecting this horizontal line.
    const std::pair<EdgeIter, EdgeIter> range(
        std::equal_range(m_edgeComponents.begin(), m_edgeComponents.end(), y, EdgeOrderY()));

    if (range.first == range.second) {
      continue;
    }

    std::copy(range.first, range.second, std::back_inserter(edges_for_line));

    // Calculate the intersection point of each edge with
    // the current horizontal line.
    for (EdgeComponent& ecomp : edges_for_line) {
      ecomp.setX(ecomp.edge().xForY(y));
    }
    // Sort edge components by the x value of the intersection point.
    std::sort(edges_for_line.begin(), edges_for_line.end(), EdgeOrderX());

    if (m_fillRule == Qt::OddEvenFill) {
      oddEvenLineBinary(&edges_for_line.front(), static_cast<int>(edges_for_line.size()), line, pattern);
    } else {
      windingLineBinary(&edges_for_line.front(), static_cast<int>(edges_for_line.size()), line, pattern, m_invert);
    }
  }
}  // PolygonRasterizer::Rasterizer::fillBinary

void PolygonRasterizer::Rasterizer::fillGrayscale(QImage& image, const uint8_t color) const {
  std::vector<EdgeComponent> edges_for_line;
  typedef std::vector<EdgeComponent>::const_iterator EdgeIter;

  uint8_t* line = image.bits();
  const int bpl = image.bytesPerLine();

  int i = qRound(m_boundingBox.top());
  line += i * bpl;
  const int limit = qRound(m_boundingBox.bottom());
  for (; i < limit; ++i, line += bpl, edges_for_line.clear()) {
    const double y = i + 0.5;

    // Get edges intersecting this horizontal line.
    const std::pair<EdgeIter, EdgeIter> range(
        std::equal_range(m_edgeComponents.begin(), m_edgeComponents.end(), y, EdgeOrderY()));

    if (range.first == range.second) {
      continue;
    }

    std::copy(range.first, range.second, std::back_inserter(edges_for_line));

    // Calculate the intersection point of each edge with
    // the current horizontal line.
    for (EdgeComponent& ecomp : edges_for_line) {
      ecomp.setX(ecomp.edge().xForY(y));
    }
    // Sort edge components by the x value of the intersection point.
    std::sort(edges_for_line.begin(), edges_for_line.end(), EdgeOrderX());

    if (m_fillRule == Qt::OddEvenFill) {
      oddEvenLineGrayscale(&edges_for_line.front(), static_cast<int>(edges_for_line.size()), line, color);
    } else {
      windingLineGrayscale(&edges_for_line.front(), static_cast<int>(edges_for_line.size()), line, color, m_invert);
    }
  }
}  // PolygonRasterizer::Rasterizer::fillGrayscale

void PolygonRasterizer::Rasterizer::oddEvenLineBinary(const EdgeComponent* const edges,
                                                      const int num_edges,
                                                      uint32_t* const line,
                                                      const uint32_t pattern) {
  for (int i = 0; i < num_edges - 1; i += 2) {
    const double x_from = edges[i].x();
    const double x_to = edges[i + 1].x();
    fillBinarySegment(qRound(x_from), qRound(x_to), line, pattern);
  }
}

void PolygonRasterizer::Rasterizer::oddEvenLineGrayscale(const EdgeComponent* const edges,
                                                         const int num_edges,
                                                         uint8_t* const line,
                                                         const uint8_t color) {
  for (int i = 0; i < num_edges - 1; i += 2) {
    const int from = qRound(edges[i].x());
    const int to = qRound(edges[i + 1].x());
    memset(line + from, color, to - from);
  }
}

void PolygonRasterizer::Rasterizer::windingLineBinary(const EdgeComponent* const edges,
                                                      const int num_edges,
                                                      uint32_t* const line,
                                                      const uint32_t pattern,
                                                      bool invert) {
  int dir_sum = 0;
  for (int i = 0; i < num_edges - 1; ++i) {
    dir_sum += edges[i].edge().vertDirection();
    if ((dir_sum == 0) == invert) {
      const double x_from = edges[i].x();
      const double x_to = edges[i + 1].x();
      fillBinarySegment(qRound(x_from), qRound(x_to), line, pattern);
    }
  }
}

void PolygonRasterizer::Rasterizer::windingLineGrayscale(const EdgeComponent* const edges,
                                                         const int num_edges,
                                                         uint8_t* const line,
                                                         const uint8_t color,
                                                         bool invert) {
  int dir_sum = 0;
  for (int i = 0; i < num_edges - 1; ++i) {
    dir_sum += edges[i].edge().vertDirection();
    if ((dir_sum == 0) == invert) {
      const int from = qRound(edges[i].x());
      const int to = qRound(edges[i + 1].x());
      memset(line + from, color, to - from);
    }
  }
}

void PolygonRasterizer::Rasterizer::fillBinarySegment(const int x_from,
                                                      const int x_to,
                                                      uint32_t* const line,
                                                      const uint32_t pattern) {
  if (x_from == x_to) {
    return;
  }

  const uint32_t full_mask = ~uint32_t(0);
  const uint32_t first_word_mask = full_mask >> (x_from & 31);
  const uint32_t last_word_mask = full_mask << (31 - ((x_to - 1) & 31));
  const int first_word_idx = x_from >> 5;
  const int last_word_idx = (x_to - 1) >> 5;  // x_to is exclusive
  if (first_word_idx == last_word_idx) {
    const uint32_t mask = first_word_mask & last_word_mask;
    uint32_t& word = line[first_word_idx];
    word = (word & ~mask) | (pattern & mask);

    return;
  }

  int i = first_word_idx;

  // First word.
  uint32_t& first_word = line[i];
  first_word = (first_word & ~first_word_mask) | (pattern & first_word_mask);

  // Middle words.
  for (++i; i < last_word_idx; ++i) {
    line[i] = pattern;
  }

  // Last word.
  uint32_t& last_word = line[i];
  last_word = (last_word & ~last_word_mask) | (pattern & last_word_mask);
}
}  // namespace imageproc
