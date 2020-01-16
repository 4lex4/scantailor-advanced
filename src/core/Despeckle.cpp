// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Despeckle.h"

#include <BinaryImage.h>
#include <ConnectivityMap.h>

#include <QDebug>
#include <QImage>
#include <cmath>
#include <unordered_map>

#include "DebugImages.h"
#include "Dpi.h"
#include "FastQueue.h"
#include "TaskStatus.h"

/**
 * \file
 * The idea of this despeckling algorithm is as follows:
 * \li The connected components that are larger than the specified threshold
 * are marked as non-garbage.
 * \li If a connected component is close enough to a non-garbage component
 * and their sizes are comparable or the non-garbage one is larger, then
 * the other one is also marked as non-garbage.
 *
 * The last step may be repeated until no new components are marked.
 * as non-garbage.
 */

using namespace imageproc;

namespace {
/**
 * We treat vertical distances differently from the horizontal ones.
 * We want horizontal proximity to have greater weight, so we
 * multiply the vertical component distances by VERTICAL_SCALE,
 * so that the distance is not:\n
 * std::sqrt(dx^2 + dy^2)\n
 * but:\n
 * std::sqrt(dx^2 + (VERTICAL_SCALE*dy)^2)\n
 * Keep in mind that we actually operate on squared distances,
 * so we don't need to take that square root.
 */
const int VERTICAL_SCALE = 2;
const int VERTICAL_SCALE_SQ = VERTICAL_SCALE * VERTICAL_SCALE;

struct Settings {
  /**
   * When multiplied by the number of pixels in a connected component,
   * gives the minimum size (in terms of the number of pixels) of a connected
   * component we may attach it to.
   */
  double minRelativeParentWeight;

  /**
   * When multiplied by the number of pixels in a connected component,
   * gives the maximum squared distance to another connected component
   * we may attach it to.
   */
  uint32_t pixelsToSqDist;

  /**
   * Defines the minimum width or height in pixels that will guarantee
   * the object won't be removed.
   */
  int bigObjectThreshold;

  static Settings get(Despeckle::Level level, const Dpi& dpi);

  static Settings get(double level, const Dpi& dpi);
};

Settings Settings::get(const Despeckle::Level level, const Dpi& dpi) {
  Settings settings{};

  const int minDpi = std::min(dpi.horizontal(), dpi.vertical());
  const double dpiFactor = minDpi / 300.0;
  // To silence compiler's warnings.
  settings.minRelativeParentWeight = 0;
  settings.pixelsToSqDist = 0;
  settings.bigObjectThreshold = 0;

  switch (level) {
    case Despeckle::CAUTIOUS:
      settings.minRelativeParentWeight = 0.125 * dpiFactor;
      settings.pixelsToSqDist = static_cast<uint32_t>(std::pow(10.0, 2));
      settings.bigObjectThreshold = qRound(7 * dpiFactor);
      break;
    case Despeckle::NORMAL:
      settings.minRelativeParentWeight = 0.175 * dpiFactor;
      settings.pixelsToSqDist = static_cast<uint32_t>(std::pow(6.5, 2));
      settings.bigObjectThreshold = qRound(12 * dpiFactor);
      break;
    case Despeckle::AGGRESSIVE:
      settings.minRelativeParentWeight = 0.225 * dpiFactor;
      settings.pixelsToSqDist = static_cast<uint32_t>(std::pow(3.5, 2));
      settings.bigObjectThreshold = qRound(17 * dpiFactor);
      break;
  }
  return settings;
}

Settings Settings::get(const double level, const Dpi& dpi) {
  Settings settings{};

  const int minDpi = std::min(dpi.horizontal(), dpi.vertical());
  const double dpiFactor = minDpi / 300.0;

  settings.minRelativeParentWeight = (0.05 * level + 0.075) * dpiFactor;
  settings.pixelsToSqDist = static_cast<uint32_t>(std::pow(0.25 * std::pow(level, 2) - 4.25 * level + 14, 2));
  settings.bigObjectThreshold = qRound((5 * level + 2) * dpiFactor);
  return settings;
}

struct Component {
  static const uint32_t ANCHORED_TO_BIG = uint32_t(1) << 31;
  static const uint32_t ANCHORED_TO_SMALL = uint32_t(1) << 30;
  static const uint32_t TAG_MASK = ANCHORED_TO_BIG | ANCHORED_TO_SMALL;

  /**
   * Lower 30 bits: the number of pixels in the connected component.
   * Higher 2 bits: tags.
   */
  uint32_t numPixels;

  Component() : numPixels(0) {}

  uint32_t anchoredToBig() const { return numPixels & ANCHORED_TO_BIG; }

  void setAnchoredToBig() { numPixels |= ANCHORED_TO_BIG; }

  uint32_t anchoredToSmall() const { return numPixels & ANCHORED_TO_SMALL; }

  void setAnchoredToSmall() { numPixels |= ANCHORED_TO_SMALL; }

  bool anchoredToSmallButNotBig() const { return (numPixels & TAG_MASK) == ANCHORED_TO_SMALL; }

  void clearTags() { numPixels &= ~TAG_MASK; }
};

const uint32_t Component::ANCHORED_TO_BIG;
const uint32_t Component::ANCHORED_TO_SMALL;
const uint32_t Component::TAG_MASK;

struct BoundingBox {
  int top;
  int left;
  int bottom;
  int right;

  BoundingBox() {
    top = left = std::numeric_limits<int>::max();
    bottom = right = std::numeric_limits<int>::min();
  }

  int width() const { return right - left + 1; }

  int height() const { return bottom - top + 1; }

  void extend(int x, int y) {
    top = std::min(top, y);
    left = std::min(left, x);
    bottom = std::max(bottom, y);
    right = std::max(right, x);
  }
};

struct Vector {
  int16_t x;
  int16_t y;
};

union Distance {
  Vector vec;
  uint32_t raw;

  static Distance zero() {
    Distance dist{};
    dist.raw = 0;
    return dist;
  }

  static Distance special() {
    Distance dist{};
    dist.vec.x = dist.vec.y = std::numeric_limits<int16_t>::max();
    return dist;
  }

  bool operator==(const Distance& other) const { return raw == other.raw; }

  bool operator!=(const Distance& other) const { return raw != other.raw; }

  void reset(int x) {
    vec.x = static_cast<int16_t>(std::numeric_limits<int16_t>::max() - x);
    vec.y = 0;
  }

  uint32_t sqdist() const {
    const int x = vec.x;
    const int y = vec.y;
    return static_cast<uint32_t>(x * x + VERTICAL_SCALE_SQ * y * y);
  }
};

/**
 * \brief A bidirectional association between two connected components.
 */
struct Connection {
  uint32_t lesserLabel;
  uint32_t greaterLabel;

  Connection(uint32_t lbl1, uint32_t lbl2) {
    if (lbl1 < lbl2) {
      lesserLabel = lbl1;
      greaterLabel = lbl2;
    } else {
      lesserLabel = lbl2;
      greaterLabel = lbl1;
    }
  }

  bool operator<(const Connection& rhs) const {
    if (lesserLabel < rhs.lesserLabel) {
      return true;
    } else if (lesserLabel > rhs.lesserLabel) {
      return false;
    } else {
      return greaterLabel < rhs.greaterLabel;
    }
  }

  bool operator==(const Connection& other) const {
    return (lesserLabel == other.lesserLabel) && (greaterLabel == other.greaterLabel);
  }

  struct hash {
    std::size_t operator()(const Connection& connection) const noexcept {
      return std::hash<int>()(connection.lesserLabel) ^ std::hash<int>()(connection.greaterLabel << 1);
    }
  };
};

/**
 * \brief A directional assiciation between two connected components.
 */
struct TargetSourceConn {
  uint32_t target;

  /**< The label of the target connected component. */
  uint32_t source;

  /**< The label of the source connected component. */

  TargetSourceConn(uint32_t tgt, uint32_t src) : target(tgt), source(src) {}

  /**
   * The ordering is by target then source.  It's designed to be able
   * to quickly locate all associations involving a specific target.
   */
  bool operator<(const TargetSourceConn& rhs) const {
    if (target < rhs.target) {
      return true;
    } else if (target > rhs.target) {
      return false;
    } else {
      return source < rhs.source;
    }
  }
};

/**
 * \brief If the association didn't exist, create it,
 *        otherwise the minimum distance.
 */
void updateDistance(std::unordered_map<Connection, uint32_t, Connection::hash>& conns,
                    uint32_t label1,
                    uint32_t label2,
                    uint32_t sqdist) {
  using Connections = std::unordered_map<Connection, uint32_t, Connection::hash>;

  const Connection conn(label1, label2);
  auto it(conns.find(conn));
  if (it == conns.end()) {
    conns.insert(Connections::value_type(conn, sqdist));
  } else if (sqdist < it->second) {
    it->second = sqdist;
  }
}

/**
 * \brief Tag the source component with ANCHORED_TO_SMALL, ANCHORED_TO_BIG
 *        or none of the above.
 */
void tagSourceComponent(Component& source, const Component& target, uint32_t sqdist, const Settings& settings) {
  if (source.anchoredToBig()) {
    // No point in setting ANCHORED_TO_SMALL.
    return;
  }

  if (sqdist > source.numPixels * settings.pixelsToSqDist) {
    // Too far.
    return;
  }

  if (target.numPixels >= settings.minRelativeParentWeight * source.numPixels) {
    source.setAnchoredToBig();
  } else {
    source.setAnchoredToSmall();
  }
}

/**
 * Check if the component may be attached to another one.
 * Attaching a component to another one will preserve the component
 * being attached, provided that the one it's attached to is also preserved.
 */
bool canBeAttachedTo(const Component& comp, const Component& target, uint32_t sqdist, const Settings& settings) {
  if (sqdist <= comp.numPixels * settings.pixelsToSqDist) {
    if (target.numPixels >= comp.numPixels * settings.minRelativeParentWeight) {
      return true;
    }
  }
  return false;
}

void voronoi(ConnectivityMap& cmap, std::vector<Distance>& dist) {
  const int width = cmap.size().width() + 2;
  const int height = cmap.size().height() + 2;

  assert(dist.empty());
  dist.resize(width * height, Distance::zero());

  std::vector<uint32_t> sqdists(width * 2, 0);
  uint32_t* prevSqdistLine = &sqdists[0];
  uint32_t* thisSqdistLine = &sqdists[width];

  Distance* distLine = &dist[0];
  uint32_t* cmapLine = cmap.paddedData();

  distLine[0].reset(0);
  prevSqdistLine[0] = distLine[0].sqdist();
  for (int x = 1; x < width; ++x) {
    distLine[x].vec.x = static_cast<int16_t>(distLine[x - 1].vec.x - 1);
    prevSqdistLine[x] = prevSqdistLine[x - 1] - (int(distLine[x - 1].vec.x) << 1) + 1;
  }

  // Top to bottom scan.
  for (int y = 1; y < height; ++y) {
    distLine += width;
    cmapLine += width;
    distLine[0].reset(0);
    distLine[width - 1].reset(width - 1);
    thisSqdistLine[0] = distLine[0].sqdist();
    thisSqdistLine[width - 1] = distLine[width - 1].sqdist();
    // Left to right scan.
    for (int x = 1; x < width - 1; ++x) {
      if (cmapLine[x]) {
        thisSqdistLine[x] = 0;
        assert(distLine[x] == Distance::zero());
        continue;
      }

      // Propagate from left.
      Distance leftDist = distLine[x - 1];
      uint32_t sqdistLeft = thisSqdistLine[x - 1];
      sqdistLeft += 1 - (int(leftDist.vec.x) << 1);
      // Propagate from top.
      Distance topDist = distLine[x - width];
      uint32_t sqdistTop = prevSqdistLine[x];
      sqdistTop += VERTICAL_SCALE_SQ - 2 * VERTICAL_SCALE_SQ * int(topDist.vec.y);

      if (sqdistLeft < sqdistTop) {
        thisSqdistLine[x] = sqdistLeft;
        --leftDist.vec.x;
        distLine[x] = leftDist;
        cmapLine[x] = cmapLine[x - 1];
      } else {
        thisSqdistLine[x] = sqdistTop;
        --topDist.vec.y;
        distLine[x] = topDist;
        cmapLine[x] = cmapLine[x - width];
      }
    }

    // Right to left scan.
    for (int x = width - 2; x >= 1; --x) {
      // Propagate from right.
      Distance rightDist = distLine[x + 1];
      uint32_t sqdistRight = thisSqdistLine[x + 1];
      sqdistRight += 1 + (int(rightDist.vec.x) << 1);

      if (sqdistRight < thisSqdistLine[x]) {
        thisSqdistLine[x] = sqdistRight;
        ++rightDist.vec.x;
        distLine[x] = rightDist;
        cmapLine[x] = cmapLine[x + 1];
      }
    }

    std::swap(thisSqdistLine, prevSqdistLine);
  }

  // Bottom to top scan.
  for (int y = height - 2; y >= 1; --y) {
    distLine -= width;
    cmapLine -= width;
    distLine[0].reset(0);
    distLine[width - 1].reset(width - 1);
    thisSqdistLine[0] = distLine[0].sqdist();
    thisSqdistLine[width - 1] = distLine[width - 1].sqdist();
    // Right to left scan.
    for (int x = width - 2; x >= 1; --x) {
      // Propagate from right.
      Distance rightDist = distLine[x + 1];
      uint32_t sqdistRight = thisSqdistLine[x + 1];
      sqdistRight += 1 + (int(rightDist.vec.x) << 1);
      // Propagate from bottom.
      Distance bottomDist = distLine[x + width];
      uint32_t sqdistBottom = prevSqdistLine[x];
      sqdistBottom += VERTICAL_SCALE_SQ + 2 * VERTICAL_SCALE_SQ * int(bottomDist.vec.y);

      thisSqdistLine[x] = distLine[x].sqdist();

      if (sqdistRight < thisSqdistLine[x]) {
        thisSqdistLine[x] = sqdistRight;
        ++rightDist.vec.x;
        distLine[x] = rightDist;
        assert(cmapLine[x] == 0 || cmapLine[x + 1] != 0);
        cmapLine[x] = cmapLine[x + 1];
      }
      if (sqdistBottom < thisSqdistLine[x]) {
        thisSqdistLine[x] = sqdistBottom;
        ++bottomDist.vec.y;
        distLine[x] = bottomDist;
        assert(cmapLine[x] == 0 || cmapLine[x + width] != 0);
        cmapLine[x] = cmapLine[x + width];
      }
    }
    // Left to right scan.
    for (int x = 1; x < width - 1; ++x) {
      // Propagate from left.
      Distance leftDist = distLine[x - 1];
      uint32_t sqdistLeft = thisSqdistLine[x - 1];
      sqdistLeft += 1 - (int(leftDist.vec.x) << 1);

      if (sqdistLeft < thisSqdistLine[x]) {
        thisSqdistLine[x] = sqdistLeft;
        --leftDist.vec.x;
        distLine[x] = leftDist;
        assert(cmapLine[x] == 0 || cmapLine[x - 1] != 0);
        cmapLine[x] = cmapLine[x - 1];
      }
    }

    std::swap(thisSqdistLine, prevSqdistLine);
  }
}  // voronoi

void voronoiSpecial(ConnectivityMap& cmap, std::vector<Distance>& dist, const Distance specialDistance) {
  const int width = cmap.size().width() + 2;
  const int height = cmap.size().height() + 2;

  std::vector<uint32_t> sqdists(width * 2, 0);
  uint32_t* prevSqdistLine = &sqdists[0];
  uint32_t* thisSqdistLine = &sqdists[width];

  Distance* distLine = &dist[0];
  uint32_t* cmapLine = cmap.paddedData();

  distLine[0].reset(0);
  prevSqdistLine[0] = distLine[0].sqdist();
  for (int x = 1; x < width; ++x) {
    distLine[x].vec.x = static_cast<int16_t>(distLine[x - 1].vec.x - 1);
    prevSqdistLine[x] = prevSqdistLine[x - 1] - (int(distLine[x - 1].vec.x) << 1) + 1;
  }

  // Top to bottom scan.
  for (int y = 1; y < height - 1; ++y) {
    distLine += width;
    cmapLine += width;
    distLine[0].reset(0);
    distLine[width - 1].reset(width - 1);
    thisSqdistLine[0] = distLine[0].sqdist();
    thisSqdistLine[width - 1] = distLine[width - 1].sqdist();
    // Left to right scan.
    for (int x = 1; x < width - 1; ++x) {
      if (distLine[x] == specialDistance) {
        continue;
      }

      thisSqdistLine[x] = distLine[x].sqdist();
      // Propagate from left.
      Distance leftDist = distLine[x - 1];
      if (leftDist != specialDistance) {
        uint32_t sqdistLeft = thisSqdistLine[x - 1];
        sqdistLeft += 1 - (int(leftDist.vec.x) << 1);
        if (sqdistLeft < thisSqdistLine[x]) {
          thisSqdistLine[x] = sqdistLeft;
          --leftDist.vec.x;
          distLine[x] = leftDist;
          assert(cmapLine[x] == 0 || cmapLine[x - 1] != 0);
          cmapLine[x] = cmapLine[x - 1];
        }
      }
      // Propagate from top.
      Distance topDist = distLine[x - width];
      if (topDist != specialDistance) {
        uint32_t sqdistTop = prevSqdistLine[x];
        sqdistTop += VERTICAL_SCALE_SQ - 2 * VERTICAL_SCALE_SQ * int(topDist.vec.y);
        if (sqdistTop < thisSqdistLine[x]) {
          thisSqdistLine[x] = sqdistTop;
          --topDist.vec.y;
          distLine[x] = topDist;
          assert(cmapLine[x] == 0 || cmapLine[x - width] != 0);
          cmapLine[x] = cmapLine[x - width];
        }
      }
    }

    // Right to left scan.
    for (int x = width - 2; x >= 1; --x) {
      if (distLine[x] == specialDistance) {
        continue;
      }
      // Propagate from right.
      Distance rightDist = distLine[x + 1];
      if (rightDist != specialDistance) {
        uint32_t sqdistRight = thisSqdistLine[x + 1];
        sqdistRight += 1 + (int(rightDist.vec.x) << 1);
        if (sqdistRight < thisSqdistLine[x]) {
          thisSqdistLine[x] = sqdistRight;
          ++rightDist.vec.x;
          distLine[x] = rightDist;
          assert(cmapLine[x] == 0 || cmapLine[x + 1] != 0);
          cmapLine[x] = cmapLine[x + 1];
        }
      }
    }

    std::swap(thisSqdistLine, prevSqdistLine);
  }

  // Bottom to top scan.
  for (int y = height - 2; y >= 1; --y) {
    distLine -= width;
    cmapLine -= width;
    distLine[0].reset(0);
    distLine[width - 1].reset(width - 1);
    thisSqdistLine[0] = distLine[0].sqdist();
    thisSqdistLine[width - 1] = distLine[width - 1].sqdist();
    // Right to left scan.
    for (int x = width - 2; x >= 1; --x) {
      if (distLine[x] == specialDistance) {
        continue;
      }

      thisSqdistLine[x] = distLine[x].sqdist();
      // Propagate from right.
      Distance rightDist = distLine[x + 1];
      if (rightDist != specialDistance) {
        uint32_t sqdistRight = thisSqdistLine[x + 1];
        sqdistRight += 1 + (int(rightDist.vec.x) << 1);
        if (sqdistRight < thisSqdistLine[x]) {
          thisSqdistLine[x] = sqdistRight;
          ++rightDist.vec.x;
          distLine[x] = rightDist;
          assert(cmapLine[x] == 0 || cmapLine[x + 1] != 0);
          cmapLine[x] = cmapLine[x + 1];
        }
      }
      // Propagate from bottom.
      Distance bottomDist = distLine[x + width];
      if (bottomDist != specialDistance) {
        uint32_t sqdistBottom = prevSqdistLine[x];
        sqdistBottom += VERTICAL_SCALE_SQ + 2 * VERTICAL_SCALE_SQ * int(bottomDist.vec.y);
        if (sqdistBottom < thisSqdistLine[x]) {
          thisSqdistLine[x] = sqdistBottom;
          ++bottomDist.vec.y;
          distLine[x] = bottomDist;
          assert(cmapLine[x] == 0 || cmapLine[x + width] != 0);
          cmapLine[x] = cmapLine[x + width];
        }
      }
    }

    // Left to right scan.
    for (int x = 1; x < width - 1; ++x) {
      if (distLine[x] == specialDistance) {
        continue;
      }
      // Propagate from left.
      Distance leftDist = distLine[x - 1];
      if (leftDist != specialDistance) {
        uint32_t sqdistLeft = thisSqdistLine[x - 1];
        sqdistLeft += 1 - (int(leftDist.vec.x) << 1);
        if (sqdistLeft < thisSqdistLine[x]) {
          thisSqdistLine[x] = sqdistLeft;
          --leftDist.vec.x;
          distLine[x] = leftDist;
          assert(cmapLine[x] == 0 || cmapLine[x - 1] != 0);
          cmapLine[x] = cmapLine[x - 1];
        }
      }
    }

    std::swap(thisSqdistLine, prevSqdistLine);
  }
}  // voronoiSpecial

/**
 * Calculate the minimum distance between components from neighboring
 * Voronoi segments.
 */
void voronoiDistances(const ConnectivityMap& cmap,
                      const std::vector<Distance>& distanceMatrix,
                      std::unordered_map<Connection, uint32_t, Connection::hash>& conns) {
  const int width = cmap.size().width();
  const int height = cmap.size().height();

  const int offsets[] = {-cmap.stride(), -1, 1, cmap.stride()};

  const uint32_t* const cmapData = cmap.data();
  const Distance* const distanceData = &distanceMatrix[0] + width + 3;
  for (int y = 0, offset = 0; y < height; ++y, offset += 2) {
    for (int x = 0; x < width; ++x, ++offset) {
      const uint32_t label = cmapData[offset];
      assert(label != 0);

      const int x1 = x + distanceData[offset].vec.x;
      const int y1 = y + distanceData[offset].vec.y;

      for (int i : offsets) {
        const int nbhOffset = offset + i;
        const uint32_t nbhLabel = cmapData[nbhOffset];
        if ((nbhLabel == 0) || (nbhLabel == label)) {
          // label 0 can be encountered in
          // padding lines.
          continue;
        }

        const int x2 = x + distanceData[nbhOffset].vec.x;
        const int y2 = y + distanceData[nbhOffset].vec.y;
        const int dx = x1 - x2;
        const int dy = y1 - y2;
        const uint32_t sqdist = dx * dx + dy * dy;

        updateDistance(conns, label, nbhLabel, sqdist);
      }
    }
  }
}  // voronoiDistances

void despeckleImpl(BinaryImage& image,
                   const Dpi& dpi,
                   const Settings& settings,
                   const TaskStatus& status,
                   DebugImages* const dbg) {
  ConnectivityMap cmap(image, CONN8);
  if (cmap.maxLabel() == 0) {
    // Completely white image?
    return;
  }

  status.throwIfCancelled();

  std::vector<Component> components(cmap.maxLabel() + 1);
  std::vector<BoundingBox> boundingBoxes(cmap.maxLabel() + 1);

  const int width = image.width();
  const int height = image.height();

  uint32_t* const cmapData = cmap.data();

  // Count the number of pixels and a bounding rect of each component.
  uint32_t* cmapLine = cmapData;
  const int cmapStride = cmap.stride();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = cmapLine[x];
      ++components[label].numPixels;
      boundingBoxes[label].extend(x, y);
    }
    cmapLine += cmapStride;
  }

  status.throwIfCancelled();

  // Unify big components into one.
  std::vector<uint32_t> remappingTable(components.size());
  uint32_t unifiedBigComponent = 0;
  uint32_t nextAvailComponent = 1;
  for (uint32_t label = 1; label <= cmap.maxLabel(); ++label) {
    if ((boundingBoxes[label].width() < settings.bigObjectThreshold)
        && (boundingBoxes[label].height() < settings.bigObjectThreshold)) {
      components[nextAvailComponent] = components[label];
      remappingTable[label] = nextAvailComponent;
      ++nextAvailComponent;
    } else {
      if (unifiedBigComponent == 0) {
        unifiedBigComponent = nextAvailComponent;
        ++nextAvailComponent;
        components[unifiedBigComponent] = components[label];
        // Set numPixels to a large value so that canBeAttachedTo()
        // always allows attaching to any such component.
        components[unifiedBigComponent].numPixels = width * height;
      }
      remappingTable[label] = unifiedBigComponent;
    }
  }
  components.resize(nextAvailComponent);
  std::vector<BoundingBox>().swap(boundingBoxes);  // We don't need them any more.
  status.throwIfCancelled();

  const uint32_t maxLabel = nextAvailComponent - 1;
  // Remapping individual pixels.
  cmapLine = cmapData;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      cmapLine[x] = remappingTable[cmapLine[x]];
    }
    cmapLine += cmapStride;
  }
  if (dbg) {
    dbg->add(cmap.visualized(), "big_components_unified");
  }

  status.throwIfCancelled();
  // Build a Voronoi diagram.
  std::vector<Distance> distanceMatrix;
  voronoi(cmap, distanceMatrix);
  if (dbg) {
    dbg->add(cmap.visualized(), "voronoi");
  }

  status.throwIfCancelled();

  Distance* const distanceData = &distanceMatrix[0] + width + 3;

  // Now build a bidirectional map of distances between neighboring
  // connected components.

  using Connections = std::unordered_map<Connection, uint32_t, Connection::hash>;  // conn -> sqdist
  Connections conns;

  voronoiDistances(cmap, distanceMatrix, conns);

  status.throwIfCancelled();

  // Tag connected components with ANCHORED_TO_BIG or ANCHORED_TO_SMALL.
  for (const Connections::value_type& pair : conns) {
    const Connection conn(pair.first);
    const uint32_t sqdist = pair.second;
    Component& comp1 = components[conn.lesserLabel];
    Component& comp2 = components[conn.greaterLabel];
    tagSourceComponent(comp1, comp2, sqdist, settings);
    tagSourceComponent(comp2, comp1, sqdist, settings);
  }

  // Prevent it from growing when we compute the Voronoi diagram
  // the second time.
  components[unifiedBigComponent].setAnchoredToBig();

  bool haveAnchoredToSmallButNotBig = false;
  for (const Component& comp : components) {
    if (comp.anchoredToSmallButNotBig()) {
      haveAnchoredToSmallButNotBig = true;
      break;
    }
  }

  if (haveAnchoredToSmallButNotBig) {
    status.throwIfCancelled();

    // Give such components a second chance.  Maybe they do have
    // big neighbors, but Voronoi regions from a smaller ones
    // block the path to the bigger ones.

    const Distance zeroDistance(Distance::zero());
    const Distance specialDistance(Distance::special());
    for (int y = 0, offset = 0; y < height; ++y, offset += 2) {
      for (int x = 0; x < width; ++x, ++offset) {
        const uint32_t label = cmapData[offset];
        assert(label != 0);

        const Component& comp = components[label];
        if (!comp.anchoredToSmallButNotBig()) {
          if (distanceData[offset] == zeroDistance) {
            // Prevent this region from growing
            // and from being taken over by another
            // by another region.
            distanceData[offset] = specialDistance;
          } else {
            // Allow this region to be taken over by others.
            // Note: x + 1 here is equivalent to x
            // in voronoi() or voronoiSpecial().
            distanceData[offset].reset(x + 1);
          }
        }
      }
    }

    status.throwIfCancelled();

    // Calculate the Voronoi diagram again, but this time
    // treat pixels with a special distance in such a way
    // to prevent them from spreading but also preventing
    // them from being overwritten.
    voronoiSpecial(cmap, distanceMatrix, specialDistance);
    if (dbg) {
      dbg->add(cmap.visualized(), "voronoi_special");
    }

    status.throwIfCancelled();

    // We've got new connections.  Add them to the map.
    voronoiDistances(cmap, distanceMatrix, conns);
  }

  status.throwIfCancelled();

  // Clear the distance matrix.
  std::vector<Distance>().swap(distanceMatrix);

  // Remove tags from components.
  for (Component& comp : components) {
    comp.clearTags();
  }
  // Build a directional connection map and only include
  // good connections, that is those with a small enough
  // distance.
  // While at it, clear the bidirectional connection map.
  std::vector<TargetSourceConn> targetSource;
  while (!conns.empty()) {
    const auto it(conns.begin());
    const uint32_t label1 = it->first.lesserLabel;
    const uint32_t label2 = it->first.greaterLabel;
    const uint32_t sqdist = it->second;
    const Component& comp1 = components[label1];
    const Component& comp2 = components[label2];
    if (canBeAttachedTo(comp1, comp2, sqdist, settings)) {
      targetSource.emplace_back(label2, label1);
    }
    if (canBeAttachedTo(comp2, comp1, sqdist, settings)) {
      targetSource.emplace_back(label1, label2);
    }
    conns.erase(it);
  }

  std::sort(targetSource.begin(), targetSource.end());

  status.throwIfCancelled();

  // Create an index for quick access to a group of connections
  // with a specified target.
  std::vector<size_t> targetSourceIdx;
  const size_t numTargetSources = targetSource.size();
  uint32_t prevLabel = uint32_t(0) - 1;
  for (size_t i = 0; i < numTargetSources; ++i) {
    const TargetSourceConn& conn = targetSource[i];
    assert(conn.target != 0);
    for (; prevLabel != conn.target; ++prevLabel) {
      targetSourceIdx.push_back(i);
    }
    assert(targetSourceIdx.size() - 1 == conn.target);
  }
  for (auto label = static_cast<uint32_t>(targetSourceIdx.size()); label <= maxLabel; ++label) {
    targetSourceIdx.push_back(numTargetSources);
  }
  // Labels of components that are to be retained.
  FastQueue<uint32_t> okLabels;
  okLabels.push(unifiedBigComponent);

  while (!okLabels.empty()) {
    const uint32_t label = okLabels.front();
    okLabels.pop();

    Component& comp = components[label];
    if (comp.anchoredToBig()) {
      continue;
    }

    comp.setAnchoredToBig();

    size_t idx = targetSourceIdx[label];
    while (idx < numTargetSources && targetSource[idx].target == label) {
      okLabels.push(targetSource[idx].source);
      ++idx;
    }
  }

  status.throwIfCancelled();
  // Remove unmarked components from the binary image.
  const uint32_t msb = uint32_t(1) << 31;
  uint32_t* imageLine = image.data();
  const int imageStride = image.wordsPerLine();
  cmapLine = cmapData;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!components[cmapLine[x]].anchoredToBig()) {
        imageLine[x >> 5] &= ~(msb >> (x & 31));
      }
    }
    imageLine += imageStride;
    cmapLine += cmapStride;
  }
}
}  // namespace

BinaryImage Despeckle::despeckle(const BinaryImage& src,
                                 const Dpi& dpi,
                                 const Level level,
                                 const TaskStatus& status,
                                 DebugImages* const dbg) {
  BinaryImage dst(src);
  despeckleInPlace(dst, dpi, level, status, dbg);
  return dst;
}

void Despeckle::despeckleInPlace(BinaryImage& image,
                                 const Dpi& dpi,
                                 const Level level,
                                 const TaskStatus& status,
                                 DebugImages* const dbg) {
  const Settings settings = Settings::get(level, dpi);
  despeckleImpl(image, dpi, settings, status, dbg);
}

imageproc::BinaryImage Despeckle::despeckle(const imageproc::BinaryImage& src,
                                            const Dpi& dpi,
                                            const double level,
                                            const TaskStatus& status,
                                            DebugImages* dbg) {
  BinaryImage dst(src);
  despeckleInPlace(dst, dpi, level, status, dbg);
  return dst;
}

void Despeckle::despeckleInPlace(imageproc::BinaryImage& image,
                                 const Dpi& dpi,
                                 const double level,
                                 const TaskStatus& status,
                                 DebugImages* dbg) {
  const Settings settings = Settings::get(level, dpi);
  despeckleImpl(image, dpi, settings, status, dbg);
}
// Despeckle::despeckleInPlace
