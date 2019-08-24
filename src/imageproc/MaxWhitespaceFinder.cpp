// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "MaxWhitespaceFinder.h"
#include <QDebug>
#include <cassert>

namespace imageproc {
using namespace max_whitespace_finder;

namespace {
class AreaCompare {
 public:
  bool operator()(const QRect lhs, const QRect rhs) const {
    const int lhsArea = lhs.width() * lhs.height();
    const int rhsArea = rhs.width() * rhs.height();

    return lhsArea < rhsArea;
  }
};
}  // anonymous namespace

MaxWhitespaceFinder::MaxWhitespaceFinder(const BinaryImage& img, QSize minSize)
    : m_integralImg(img.size()),
      m_queuedRegions(new PriorityStorageImpl<AreaCompare>(AreaCompare())),
      m_minSize(minSize) {
  init(img);
}

void MaxWhitespaceFinder::init(const BinaryImage& img) {
  const int width = img.width();
  const int height = img.height();
  const uint32_t* line = img.data();
  const int wpl = img.wordsPerLine();

  for (int y = 0; y < height; ++y, line += wpl) {
    m_integralImg.beginRow();
    for (int x = 0; x < width; ++x) {
      const int shift = 31 - (x & 31);
      m_integralImg.push((line[x >> 5] >> shift) & 1);
    }
  }

  Region region(0, img.rect());
  m_queuedRegions->push(region);
}

void MaxWhitespaceFinder::addObstacle(const QRect& obstacle) {
  if (m_queuedRegions->size() == 1) {
    m_queuedRegions->top().addObstacle(obstacle);
  } else {
    m_newObstacles.push_back(obstacle);
  }
}

QRect MaxWhitespaceFinder::next(const ObstacleMode obstacleMode, int maxIterations) {
  while (maxIterations-- > 0 && !m_queuedRegions->empty()) {
    Region& topRegion = m_queuedRegions->top();
    Region region(topRegion);
    region.swapObstacles(topRegion);
    m_queuedRegions->pop();

    region.addNewObstacles(m_newObstacles);

    if (!region.obstacles().empty()) {
      subdivideUsingObstacles(region);
      continue;
    }

    if (m_integralImg.sum(region.bounds()) != 0) {
      subdivideUsingRaster(region);
      continue;
    }

    if (obstacleMode == AUTO_OBSTACLES) {
      m_newObstacles.push_back(region.bounds());
    }

    return region.bounds();
  }

  return QRect();
}

void MaxWhitespaceFinder::subdivideUsingObstacles(const Region& region) {
  const QRect bounds(region.bounds());
  const QRect pivotRect(findPivotObstacle(region));

  subdivide(region, bounds, pivotRect);
}

void MaxWhitespaceFinder::subdivideUsingRaster(const Region& region) {
  const QRect bounds(region.bounds());
  const QPoint pivotPixel(findBlackPixelCloseToCenter(bounds));
  const QRect pivotRect(extendBlackPixelToBlackBox(pivotPixel, bounds));

  subdivide(region, bounds, pivotRect);
}

void MaxWhitespaceFinder::subdivide(const Region& region, const QRect bounds, const QRect pivot) {
  // Area above the pivot obstacle.
  if (pivot.top() - bounds.top() >= m_minSize.height()) {
    QRect newBounds(bounds);
    newBounds.setBottom(pivot.top() - 1);  // Bottom is inclusive.
    Region newRegion(static_cast<unsigned int>(m_newObstacles.size()), newBounds);
    newRegion.addObstacles(region);
    m_queuedRegions->push(newRegion);
  }

  // Area below the pivot obstacle.
  if (bounds.bottom() - pivot.bottom() >= m_minSize.height()) {
    QRect newBounds(bounds);
    newBounds.setTop(pivot.bottom() + 1);
    Region newRegion(static_cast<unsigned int>(m_newObstacles.size()), newBounds);
    newRegion.addObstacles(region);
    m_queuedRegions->push(newRegion);
  }

  // Area to the left of the pivot obstacle.
  if (pivot.left() - bounds.left() >= m_minSize.width()) {
    QRect newBounds(bounds);
    newBounds.setRight(pivot.left() - 1);  // Right is inclusive.
    Region newRegion(static_cast<unsigned int>(m_newObstacles.size()), newBounds);
    newRegion.addObstacles(region);
    m_queuedRegions->push(newRegion);
  }
  // Area to the right of the pivot obstacle.
  if (bounds.right() - pivot.right() >= m_minSize.width()) {
    QRect newBounds(bounds);
    newBounds.setLeft(pivot.right() + 1);
    Region newRegion(static_cast<unsigned int>(m_newObstacles.size()), newBounds);
    newRegion.addObstacles(region);
    m_queuedRegions->push(newRegion);
  }
}  // MaxWhitespaceFinder::subdivide

QRect MaxWhitespaceFinder::findPivotObstacle(const Region& region) const {
  assert(!region.obstacles().empty());

  const QPoint center(region.bounds().center());

  QRect bestObstacle;
  int bestDistance = std::numeric_limits<int>::max();
  for (const QRect& obstacle : region.obstacles()) {
    const QPoint vec(center - obstacle.center());
    const int distance = vec.x() * vec.x() + vec.y() * vec.y();
    if (distance <= bestDistance) {
      bestObstacle = obstacle;
      bestDistance = distance;
    }
  }

  return bestObstacle;
}

QPoint MaxWhitespaceFinder::findBlackPixelCloseToCenter(const QRect nonWhiteRect) const {
  assert(m_integralImg.sum(nonWhiteRect) != 0);

  const QPoint center(nonWhiteRect.center());
  QRect outerRect(nonWhiteRect);
  QRect innerRect(center.x(), center.y(), 1, 1);

  if (m_integralImg.sum(innerRect) != 0) {
    return center;
  }

  // We have two rectangles: the outer one, that always contains at least
  // one black pixel, and the inner one (contained within the outer one),
  // that doesn't contain any black pixels.

  // The first thing we do is bringing those two rectangles as close
  // as possible to each other, so that no more than 1 pixel separates
  // their corresponding edges.

  while (true) {
    const int outerInnerDw = outerRect.width() - innerRect.width();
    const int outerInnerDh = outerRect.height() - innerRect.height();

    if ((outerInnerDw <= 1) && (outerInnerDh <= 1)) {
      break;
    }

    const int deltaLeft = innerRect.left() - outerRect.left();
    const int deltaRight = outerRect.right() - innerRect.right();
    const int deltaTop = innerRect.top() - outerRect.top();
    const int deltaBottom = outerRect.bottom() - innerRect.bottom();

    QRect middleRect(outerRect.left() + ((deltaLeft + 1) >> 1), outerRect.top() + ((deltaTop + 1) >> 1), 0, 0);
    middleRect.setRight(outerRect.right() - (deltaRight >> 1));
    middleRect.setBottom(outerRect.bottom() - (deltaBottom >> 1));
    assert(outerRect.contains(middleRect));
    assert(middleRect.contains(innerRect));

    if (m_integralImg.sum(middleRect) == 0) {
      innerRect = middleRect;
    } else {
      outerRect = middleRect;
    }
  }

  // Process the left edge.
  if (outerRect.left() != innerRect.left()) {
    QRect rect(outerRect);
    rect.setRight(rect.left());  // Right is inclusive.
    const unsigned sum = m_integralImg.sum(rect);
    if (outerRect.height() == 1) {
      // This means we are dealing with a horizontal line
      // and that we only have to check at most two pixels
      // (the endpoints) and that at least one of them
      // is definately black and that rect is a 1x1 pixels
      // block pointing to the left endpoint.
      if (sum != 0) {
        return outerRect.topLeft();
      } else {
        return outerRect.topRight();
      }
    } else if (sum != 0) {
      return findBlackPixelCloseToCenter(rect);
    }
  }
  // Process the right edge.
  if (outerRect.right() != innerRect.right()) {
    QRect rect(outerRect);
    rect.setLeft(rect.right());  // Right is inclusive.
    const unsigned sum = m_integralImg.sum(rect);
    if (outerRect.height() == 1) {
      // Same as above, except rect now points to the
      // right endpoint.
      if (sum != 0) {
        return outerRect.topRight();
      } else {
        return outerRect.topLeft();
      }
    } else if (sum != 0) {
      return findBlackPixelCloseToCenter(rect);
    }
  }

  // Process the top edge.
  if (outerRect.top() != innerRect.top()) {
    QRect rect(outerRect);
    rect.setBottom(rect.top());  // Bottom is inclusive.
    const unsigned sum = m_integralImg.sum(rect);
    if (outerRect.width() == 1) {
      // Same as above, except rect now points to the
      // top endpoint.
      if (sum != 0) {
        return outerRect.topLeft();
      } else {
        return outerRect.bottomLeft();
      }
    } else if (sum != 0) {
      return findBlackPixelCloseToCenter(rect);
    }
  }
  // Process the bottom edge.
  assert(outerRect.bottom() != innerRect.bottom());
  QRect rect(outerRect);
  rect.setTop(rect.bottom());  // Bottom is inclusive.
  assert(m_integralImg.sum(rect) != 0);
  if (outerRect.width() == 1) {
    return outerRect.bottomLeft();
  } else {
    return findBlackPixelCloseToCenter(rect);
  }
}  // MaxWhitespaceFinder::findBlackPixelCloseToCenter

QRect MaxWhitespaceFinder::extendBlackPixelToBlackBox(const QPoint pixel, const QRect bounds) const {
  assert(bounds.contains(pixel));

  QRect outerRect(bounds);
  QRect innerRect(pixel.x(), pixel.y(), 1, 1);

  if (m_integralImg.sum(outerRect) == unsigned(outerRect.width() * outerRect.height())) {
    return outerRect;
  }

  // We have two rectangles: the outer one, that always contains at least
  // one white pixel, and the inner one (contained within the outer one),
  // that doesn't.

  // We will be bringing those two rectangles as close as possible to
  // each other, so that no more than 1 pixel separates their
  // corresponding edges.

  while (true) {
    const int outerInnerDw = outerRect.width() - innerRect.width();
    const int outerInnerDh = outerRect.height() - innerRect.height();

    if ((outerInnerDw <= 1) && (outerInnerDh <= 1)) {
      break;
    }

    const int deltaLeft = innerRect.left() - outerRect.left();
    const int deltaRight = outerRect.right() - innerRect.right();
    const int deltaTop = innerRect.top() - outerRect.top();
    const int deltaBottom = outerRect.bottom() - innerRect.bottom();

    QRect middleRect(outerRect.left() + ((deltaLeft + 1) >> 1), outerRect.top() + ((deltaTop + 1) >> 1), 0, 0);
    middleRect.setRight(outerRect.right() - (deltaRight >> 1));
    middleRect.setBottom(outerRect.bottom() - (deltaBottom >> 1));
    assert(outerRect.contains(middleRect));
    assert(middleRect.contains(innerRect));

    const unsigned area = middleRect.width() * middleRect.height();
    if (m_integralImg.sum(middleRect) == area) {
      innerRect = middleRect;
    } else {
      outerRect = middleRect;
    }
  }

  return innerRect;
}  // MaxWhitespaceFinder::extendBlackPixelToBlackBox

/*======================= MaxWhitespaceFinder::Region =====================*/

MaxWhitespaceFinder::Region::Region(unsigned knownNewObstacles, const QRect& bounds)
    : m_knownNewObstacles(knownNewObstacles), m_bounds(bounds) {}

MaxWhitespaceFinder::Region::Region(const Region& other)
    : m_knownNewObstacles(other.m_knownNewObstacles), m_bounds(other.m_bounds) {
  // Note that we don't copy m_obstacles.  This is a shallow copy.
}

/**
 * Adds obstacles from another region that intersect this region's area.
 */
void MaxWhitespaceFinder::Region::addObstacles(const Region& otherRegion) {
  for (const QRect& obstacle : otherRegion.obstacles()) {
    const QRect intersected(obstacle.intersected(m_bounds));
    if (!intersected.isEmpty()) {
      m_obstacles.push_back(intersected);
    }
  }
}

/**
 * Adds global obstacles that were not there when this region was constructed.
 */
void MaxWhitespaceFinder::Region::addNewObstacles(const std::vector<QRect>& newObstacles) {
  for (size_t i = m_knownNewObstacles; i < newObstacles.size(); ++i) {
    const QRect intersected(newObstacles[i].intersected(m_bounds));
    if (!intersected.isEmpty()) {
      m_obstacles.push_back(intersected);
    }
  }
}

/**
 * A fast and non-throwing swap operation.
 */
void MaxWhitespaceFinder::Region::swap(Region& other) {
  std::swap(m_bounds, other.m_bounds);
  std::swap(m_knownNewObstacles, other.m_knownNewObstacles);
  m_obstacles.swap(other.m_obstacles);
}
}  // namespace imageproc