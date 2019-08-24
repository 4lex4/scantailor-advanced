// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  Edge(const QPointF& top, const QPointF& bottom, int vertDirection);

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
  EdgeComponent(const Edge* edge, double top, double bottom) : m_top(top), m_bottom(bottom), m_x(), m_edge(edge) {}

  double top() const { return m_top; }

  double bottom() const { return m_bottom; }

  const Edge& edge() const { return *m_edge; }

  double x() const { return m_x; }

  void setX(double x) { m_x = x; }

 private:
  double m_top;
  double m_bottom;
  double m_x;
  const Edge* m_edge;
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
  Rasterizer(const QRect& imageRect, const QPolygonF& poly, Qt::FillRule fillRule, bool invert);

  void fillBinary(BinaryImage& image, BWColor color) const;

  void fillGrayscale(QImage& image, uint8_t color) const;

 private:
  void prepareEdges();

  static void oddEvenLineBinary(const EdgeComponent* edges, int numEdges, uint32_t* line, uint32_t pattern);

  static void oddEvenLineGrayscale(const EdgeComponent* edges, int numEdges, uint8_t* line, uint8_t color);

  static void windingLineBinary(const EdgeComponent* edges,
                                int numEdges,
                                uint32_t* line,
                                uint32_t pattern,
                                bool invert);

  static void windingLineGrayscale(const EdgeComponent* edges, int numEdges, uint8_t* line, uint8_t color, bool invert);

  static void fillBinarySegment(int xFrom, int xTo, uint32_t* line, uint32_t pattern);

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
                             const Qt::FillRule fillRule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }

  Rasterizer rasterizer(image.rect(), poly, fillRule, false);
  rasterizer.fillBinary(image, color);
}

void PolygonRasterizer::fillExcept(BinaryImage& image,
                                   const BWColor color,
                                   const QPolygonF& poly,
                                   const Qt::FillRule fillRule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }

  Rasterizer rasterizer(image.rect(), poly, fillRule, true);
  rasterizer.fillBinary(image, color);
}

void PolygonRasterizer::grayFill(QImage& image,
                                 const unsigned char color,
                                 const QPolygonF& poly,
                                 const Qt::FillRule fillRule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }
  if ((image.format() != QImage::Format_Indexed8) || !image.isGrayscale()) {
    throw std::invalid_argument("PolygonRasterizer: target image is not grayscale");
  }

  Rasterizer rasterizer(image.rect(), poly, fillRule, false);
  rasterizer.fillGrayscale(image, color);
}

void PolygonRasterizer::grayFillExcept(QImage& image,
                                       const unsigned char color,
                                       const QPolygonF& poly,
                                       const Qt::FillRule fillRule) {
  if (image.isNull()) {
    throw std::invalid_argument("PolygonRasterizer: target image is null");
  }
  if ((image.format() != QImage::Format_Indexed8) || !image.isGrayscale()) {
    throw std::invalid_argument("PolygonRasterizer: target image is not grayscale");
  }

  Rasterizer rasterizer(image.rect(), poly, fillRule, true);
  rasterizer.fillGrayscale(image, color);
}

/*======================= PolygonRasterizer::Edge ==========================*/

PolygonRasterizer::Edge::Edge(const QPointF& top, const QPointF& bottom, const int vertDirection)
    : m_top(top),
      m_bottom(bottom),
      m_deltaX(bottom.x() - top.x()),
      m_reDeltaY(1.0 / (bottom.y() - top.y())),
      m_vertDirection(vertDirection) {}

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

PolygonRasterizer::Rasterizer::Rasterizer(const QRect& imageRect,
                                          const QPolygonF& poly,
                                          const Qt::FillRule fillRule,
                                          const bool invert)
    : m_imageRect(imageRect), m_fillRule(fillRule), m_invert(invert) {
  QPainterPath path1;
  path1.setFillRule(fillRule);
  path1.addRect(imageRect);

  QPainterPath path2;
  path2.setFillRule(fillRule);
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
  const int numVerts = m_fillPoly.size();
  if (numVerts == 0) {
    return;
  }

  // Collect the edges, excluding horizontal and null ones.
  m_edges.reserve(numVerts + 2);
  for (int i = 0; i < numVerts - 1; ++i) {
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
  std::vector<double> yValues;
  yValues.reserve(numVerts + 2);
  for (const QPointF& pt : m_fillPoly) {
    yValues.push_back(pt.y());
  }

  if (m_invert) {
    yValues.push_back(0.0);
    yValues.push_back(m_imageRect.height());
  }

  // Sort and remove duplicates.
  std::sort(yValues.begin(), yValues.end());
  yValues.erase(std::unique(yValues.begin(), yValues.end()), yValues.end());

  // Break edges into non-overlaping components, then sort them.
  m_edgeComponents.reserve(m_edges.size());
  for (const Edge& edge : m_edges) {
    auto it(std::lower_bound(yValues.begin(), yValues.end(), edge.topY()));

    assert(*it == edge.topY());

    do {
      auto next(it);
      ++next;
      assert(next != yValues.end());
      m_edgeComponents.emplace_back(&edge, *it, *next);
      it = next;
    } while (*it != edge.bottomY());
  }

  std::sort(m_edgeComponents.begin(), m_edgeComponents.end(), EdgeOrderY());
}  // PolygonRasterizer::Rasterizer::prepareEdges

void PolygonRasterizer::Rasterizer::fillBinary(BinaryImage& image, const BWColor color) const {
  std::vector<EdgeComponent> edgesForLine;
  using EdgeIter = std::vector<EdgeComponent>::const_iterator;

  uint32_t* line = image.data();
  const int wpl = image.wordsPerLine();
  const uint32_t pattern = (color == WHITE) ? 0 : ~uint32_t(0);

  int i = qRound(m_boundingBox.top());
  line += i * wpl;
  const int limit = qRound(m_boundingBox.bottom());
  for (; i < limit; ++i, line += wpl, edgesForLine.clear()) {
    const double y = i + 0.5;

    // Get edges intersecting this horizontal line.
    const std::pair<EdgeIter, EdgeIter> range(
        std::equal_range(m_edgeComponents.begin(), m_edgeComponents.end(), y, EdgeOrderY()));

    if (range.first == range.second) {
      continue;
    }

    std::copy(range.first, range.second, std::back_inserter(edgesForLine));

    // Calculate the intersection point of each edge with
    // the current horizontal line.
    for (EdgeComponent& ecomp : edgesForLine) {
      ecomp.setX(ecomp.edge().xForY(y));
    }
    // Sort edge components by the x value of the intersection point.
    std::sort(edgesForLine.begin(), edgesForLine.end(), EdgeOrderX());

    if (m_fillRule == Qt::OddEvenFill) {
      oddEvenLineBinary(&edgesForLine.front(), static_cast<int>(edgesForLine.size()), line, pattern);
    } else {
      windingLineBinary(&edgesForLine.front(), static_cast<int>(edgesForLine.size()), line, pattern, m_invert);
    }
  }
}  // PolygonRasterizer::Rasterizer::fillBinary

void PolygonRasterizer::Rasterizer::fillGrayscale(QImage& image, const uint8_t color) const {
  std::vector<EdgeComponent> edgesForLine;
  using EdgeIter = std::vector<EdgeComponent>::const_iterator;

  uint8_t* line = image.bits();
  const int bpl = image.bytesPerLine();

  int i = qRound(m_boundingBox.top());
  line += i * bpl;
  const int limit = qRound(m_boundingBox.bottom());
  for (; i < limit; ++i, line += bpl, edgesForLine.clear()) {
    const double y = i + 0.5;

    // Get edges intersecting this horizontal line.
    const std::pair<EdgeIter, EdgeIter> range(
        std::equal_range(m_edgeComponents.begin(), m_edgeComponents.end(), y, EdgeOrderY()));

    if (range.first == range.second) {
      continue;
    }

    std::copy(range.first, range.second, std::back_inserter(edgesForLine));

    // Calculate the intersection point of each edge with
    // the current horizontal line.
    for (EdgeComponent& ecomp : edgesForLine) {
      ecomp.setX(ecomp.edge().xForY(y));
    }
    // Sort edge components by the x value of the intersection point.
    std::sort(edgesForLine.begin(), edgesForLine.end(), EdgeOrderX());

    if (m_fillRule == Qt::OddEvenFill) {
      oddEvenLineGrayscale(&edgesForLine.front(), static_cast<int>(edgesForLine.size()), line, color);
    } else {
      windingLineGrayscale(&edgesForLine.front(), static_cast<int>(edgesForLine.size()), line, color, m_invert);
    }
  }
}  // PolygonRasterizer::Rasterizer::fillGrayscale

void PolygonRasterizer::Rasterizer::oddEvenLineBinary(const EdgeComponent* const edges,
                                                      const int numEdges,
                                                      uint32_t* const line,
                                                      const uint32_t pattern) {
  for (int i = 0; i < numEdges - 1; i += 2) {
    const double xFrom = edges[i].x();
    const double xTo = edges[i + 1].x();
    fillBinarySegment(qRound(xFrom), qRound(xTo), line, pattern);
  }
}

void PolygonRasterizer::Rasterizer::oddEvenLineGrayscale(const EdgeComponent* const edges,
                                                         const int numEdges,
                                                         uint8_t* const line,
                                                         const uint8_t color) {
  for (int i = 0; i < numEdges - 1; i += 2) {
    const int from = qRound(edges[i].x());
    const int to = qRound(edges[i + 1].x());
    memset(line + from, color, to - from);
  }
}

void PolygonRasterizer::Rasterizer::windingLineBinary(const EdgeComponent* const edges,
                                                      const int numEdges,
                                                      uint32_t* const line,
                                                      const uint32_t pattern,
                                                      bool invert) {
  int dirSum = 0;
  for (int i = 0; i < numEdges - 1; ++i) {
    dirSum += edges[i].edge().vertDirection();
    if ((dirSum == 0) == invert) {
      const double xFrom = edges[i].x();
      const double xTo = edges[i + 1].x();
      fillBinarySegment(qRound(xFrom), qRound(xTo), line, pattern);
    }
  }
}

void PolygonRasterizer::Rasterizer::windingLineGrayscale(const EdgeComponent* const edges,
                                                         const int numEdges,
                                                         uint8_t* const line,
                                                         const uint8_t color,
                                                         bool invert) {
  int dirSum = 0;
  for (int i = 0; i < numEdges - 1; ++i) {
    dirSum += edges[i].edge().vertDirection();
    if ((dirSum == 0) == invert) {
      const int from = qRound(edges[i].x());
      const int to = qRound(edges[i + 1].x());
      memset(line + from, color, to - from);
    }
  }
}

void PolygonRasterizer::Rasterizer::fillBinarySegment(const int xFrom,
                                                      const int xTo,
                                                      uint32_t* const line,
                                                      const uint32_t pattern) {
  if (xFrom == xTo) {
    return;
  }

  const uint32_t fullMask = ~uint32_t(0);
  const uint32_t firstWordMask = fullMask >> (xFrom & 31);
  const uint32_t lastWordMask = fullMask << (31 - ((xTo - 1) & 31));
  const int firstWordIdx = xFrom >> 5;
  const int lastWordIdx = (xTo - 1) >> 5;  // xTo is exclusive
  if (firstWordIdx == lastWordIdx) {
    const uint32_t mask = firstWordMask & lastWordMask;
    uint32_t& word = line[firstWordIdx];
    word = (word & ~mask) | (pattern & mask);

    return;
  }

  int i = firstWordIdx;

  // First word.
  uint32_t& firstWord = line[i];
  firstWord = (firstWord & ~firstWordMask) | (pattern & firstWordMask);

  // Middle words.
  for (++i; i < lastWordIdx; ++i) {
    line[i] = pattern;
  }

  // Last word.
  uint32_t& lastWord = line[i];
  lastWord = (lastWord & ~lastWordMask) | (pattern & lastWordMask);
}
}  // namespace imageproc
