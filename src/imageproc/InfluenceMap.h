// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_INFLUENCEMAP_H_
#define SCANTAILOR_IMAGEPROC_INFLUENCEMAP_H_

#include <QSize>
#include <cstdint>
#include <vector>

class QImage;

namespace imageproc {
class BinaryImage;
class ConnectivityMap;

/**
 * \brief Takes labelled regions and extends them to cover a larger area.
 *
 * Extension goes by taking over zero (that is background) labels.
 * Extension is done in such a way that two different labels meet at
 * a location equidistant from the initial areas of those two labels.
 */
class InfluenceMap {
 public:
  struct Vector {
    int16_t x;
    int16_t y;
  };

  struct Cell {
    /**
     * The label from the connectivity map.
     */
    uint32_t label;

    /**
     * The squared euclidean distance to the nearest pixel
     * initially (that is before extension) labelled with
     * the same label.
     */
    uint32_t distSq;

    /**
     * The vector pointing to the nearest pixel initially
     * (that is before extension) labelled with the same
     * label.
     */
    Vector vec;
  };

  /**
   * \brief Construct a null influence map.
   *
   * The data() and paddedData() methods return null on such maps.
   */
  InfluenceMap();

  /**
   * \brief Take labelled regions from a connectivity map
   *        and extend them to cover the whole available area.
   */
  explicit InfluenceMap(const ConnectivityMap& cmap);

  /**
   * \brief Take labelled regions from a connectivity map
   *        and extend them to cover the area that's black in mask.
   */
  InfluenceMap(const ConnectivityMap& cmap, const BinaryImage& mask);

  InfluenceMap(const InfluenceMap& other);

  InfluenceMap& operator=(const InfluenceMap& other);

  void swap(InfluenceMap& other);

  /**
   * \brief Returns a pointer to the top-left corner of the map.
   *
   * The data is stored in row-major order, and is padded,
   * so moving to the next line requires adding stride() rather
   * than size().width().
   */
  const Cell* data() const { return m_plainData; }

  /**
   * \brief Returns a pointer to the top-left corner of the map.
   *
   * The data is stored in row-major order, and is padded,
   * so moving to the next line requires adding stride() rather
   * than size().width().
   */
  Cell* data() { return m_plainData; }

  /**
   * \brief Returns a pointer to the top-left corner of padding of the map.
   *
   * The actually has a fake line from each side.  Those lines are
   * labelled as background (label 0).  Sometimes it might be desirable
   * to access that data.
   */
  const Cell* paddedData() const { return m_plainData ? &m_data[0] : nullptr; }

  /**
   * \brief Returns a pointer to the top-left corner of padding of the map.
   *
   * The actually has a fake line from each side.  Those lines are
   * labelled as background (label 0).  Sometimes it might be desirable
   * to access that data.
   */
  Cell* paddedData() { return m_plainData ? &m_data[0] : nullptr; }

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
   * \brief Visualizes each label with a different color.
   *
   * Label 0 (which is assigned to background) is represented
   * by transparent pixels.
   */
  QImage visualized() const;

 private:
  void init(const ConnectivityMap& cmap, const BinaryImage* mask = nullptr);

  std::vector<Cell> m_data;
  Cell* m_plainData;
  QSize m_size;
  int m_stride;
  uint32_t m_maxLabel;
};


inline void swap(InfluenceMap& o1, InfluenceMap& o2) {
  o1.swap(o2);
}
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_INFLUENCEMAP_H_
