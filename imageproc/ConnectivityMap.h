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

#ifndef IMAGEPROC_CONNECTIVITY_MAP_H_
#define IMAGEPROC_CONNECTIVITY_MAP_H_

#include <QColor>
#include <QSize>
#include <Qt>
#include <cstdint>
#include <unordered_set>
#include <vector>
#include "Connectivity.h"
#include "FastQueue.h"

class QImage;

namespace imageproc {
class BinaryImage;
class InfluenceMap;

/**
 * \brief Assigns each pixel a label that identifies the connected component
 *        it belongs to.
 *
 * Such a map makes it possible to quickly tell if a pair of pixels are
 * connected or not.
 *
 * Background (white) pixels are assigned the label of zero, and the remaining
 * labels are guaranteed not to have gaps.
 */
class ConnectivityMap {
 public:
  /**
   * \brief Constructs a null connectivity map.
   *
   * The data() and paddedData() methods return null on such maps.
   */
  ConnectivityMap();

  /**
   * \brief Constructs an empty connectivity map.
   *
   * All cells will have the label of zero.
   */
  explicit ConnectivityMap(const QSize& size);

  /**
   * \brief Labels components in a binary image.
   */
  ConnectivityMap(const BinaryImage& image, Connectivity conn);

  /**
   * \brief Same as the version working with BinaryImage
   *        but allows pixels to be represented by any data type.
   */
  template <typename T>
  ConnectivityMap(QSize size, const T* data, int units_per_line, Connectivity conn);

  ConnectivityMap(const ConnectivityMap& other);

  /**
   * \brief Constructs a connectivity map from an influence map.
   */
  explicit ConnectivityMap(const InfluenceMap& imap);

  ConnectivityMap& operator=(const ConnectivityMap& other);

  /**
   * \brief Converts an influence map to a connectivity map.
   */
  ConnectivityMap& operator=(const InfluenceMap& imap);

  void swap(ConnectivityMap& other);

  /**
   * \brief Adds another connected component and assigns it
   *        a label of maxLabel() + 1.
   *
   * The maxLabel() will then be incremented afterwards.
   *
   * It's not mandatory for a component to actually be connected.
   * In any case, all of its foreground (black) pixels will get
   * the same label.
   */
  void addComponent(const BinaryImage& image);

  /**
   * Adds another connected components which are built from the input image
   * with the given connectivity.
   *
   * The maxLabel() will become maxLabel() + N afterwards,
   * where N - number of found connected components in the input image.
   *
   * Note: this does't connect components of the current and the input map
   * even if they are actually connected.
   * Use @class InfluenceMap for that purpose.
   *
   * @param image Image with the components necessary to be added.
   * @param conn Connectivity used to build the map from the image.
   */
  void addComponents(const BinaryImage& image, Connectivity conn);

  /**
   * Adds another connected components from the another map.
   *
   * The maxLabel() will become maxLabel() + other.maxLabel() afterwards.
   *
   * Note: this does't connect components of the current and the input map
   * even if they are actually connected.
   * Use @class InfluenceMap for that purpose.
   *
   * @param other Map containing the components necessary to be added.
   */
  void addComponents(const ConnectivityMap& other);

  /**
   * Removes the connected components with the given labels.
   *
   * The maxLabel() will become maxLabel() - N afterwards,
   * where N - the number of actually removed connected components.
   *
   * @param labelsSet Set of labels determining the components which need to be removed.
   */
  void removeComponents(const std::unordered_set<uint32_t>& labelsSet);

  /**
   * Returns the mask of the current map.
   * The mask returned have white background
   * and all the labeled connected components are black.
   *
   * @return Binary mask of this connected map.
   */
  BinaryImage getBinaryMask() const;

  /**
   * \brief Returns a pointer to the top-left corner of the map.
   *
   * The data is stored in row-major order, and is padded,
   * so moving to the next line requires adding stride() rather
   * than size().width().
   */
  const uint32_t* data() const { return m_pData; }

  /**
   * \brief Returns a pointer to the top-left corner of the map.
   *
   * The data is stored in row-major order, and is padded,
   * so moving to the next line requires adding stride() rather
   * than size().width().
   */
  uint32_t* data() { return m_pData; }

  /**
   * \brief Returns a pointer to the top-left corner of padding of the map.
   *
   * The actually has a fake line from each side.  Those lines are
   * labelled as background (label 0).  Sometimes it might be desirable
   * to access that data.
   */
  const uint32_t* paddedData() const { return m_pData ? &m_data[0] : nullptr; }

  /**
   * \brief Returns a pointer to the top-left corner of padding of the map.
   *
   * The actually has a fake line from each side.  Those lines are
   * labelled as background (label 0).  Sometimes it might be desirable
   * to access that data.
   */
  uint32_t* paddedData() { return m_pData ? &m_data[0] : nullptr; }

  /**
   * \brief Returns non-padded dimensions of the map.
   */
  QSize size() const { return m_size; }

  /**
   * \brief Returns the number of units on a padded line.
   *
   * Whether working with padded or non-padded maps, adding
   * this number to a data pointer will move it one line down.
   */
  int stride() const { return m_stride; }

  /**
   * \brief Returns the maximum label present on the map.
   */
  uint32_t maxLabel() const { return m_maxLabel; }

  /**
   * Updating the maximum label may be necessary after manually
   * altering the map.
   */
  void setMaxLabel(uint32_t max_label) { m_maxLabel = max_label; }

  /**
   * \brief Visualizes each label with a different color.
   *
   * \param bg_color Background color.  Transparency is supported.
   */
  QImage visualized(QColor bg_color = Qt::black) const;

 private:
  void copyFromInfluenceMap(const InfluenceMap& imap);

  void assignIds(Connectivity conn);

  uint32_t initialTagging();

  void spreadMin4();

  void spreadMin8();

  void processNeighbor(FastQueue<uint32_t*>& queue, uint32_t this_val, uint32_t* neighbor);

  void processQueue4(FastQueue<uint32_t*>& queue);

  void processQueue8(FastQueue<uint32_t*>& queue);

  void markUsedIds(std::vector<uint32_t>& used_map) const;

  void remapIds(const std::vector<uint32_t>& map);

  static const uint32_t BACKGROUND;
  static const uint32_t UNTAGGED_FG;

  std::vector<uint32_t> m_data;
  uint32_t* m_pData;
  QSize m_size;
  int m_stride;
  uint32_t m_maxLabel;
};


inline void swap(ConnectivityMap& o1, ConnectivityMap& o2) {
  o1.swap(o2);
}

template <typename T>
ConnectivityMap::ConnectivityMap(const QSize size, const T* src, const int src_stride, const Connectivity conn)
    : m_pData(0), m_size(size), m_stride(0), m_maxLabel(0) {
  if (size.isEmpty()) {
    return;
  }

  const int width = size.width();
  const int height = size.height();

  m_data.resize((width + 2) * (height + 2), BACKGROUND);
  m_stride = width + 2;
  m_pData = &m_data[0] + 1 + m_stride;

  uint32_t* dst = m_pData;
  const int dst_stride = m_stride;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (src[x] != T()) {
        dst[x] = UNTAGGED_FG;
      }
    }
    src += src_stride;
    dst += dst_stride;
  }

  assignIds(conn);
}
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_CONNECTIVITY_MAP_H_
