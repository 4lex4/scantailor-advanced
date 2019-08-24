// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_GRID_H_
#define SCANTAILOR_FOUNDATION_GRID_H_

#include <boost/scoped_array.hpp>

template <typename Node>
class Grid {
 public:
  /**
   * Creates a null grid.
   */
  Grid();

  /**
   * \brief Creates a width x height grid with specified padding on each side.
   */
  Grid(int width, int height, int padding);

  /**
   * \brief Creates a deep copy of another grid including padding.
   *
   * Stride is also preserved.
   */
  Grid(const Grid& other);

  bool isNull() const { return m_width <= 0 || m_height <= 0; }

  void initPadding(const Node& paddingNode);

  void initInterior(const Node& interiorNode);

  /**
   * \brief Returns a pointer to the beginning of unpadded data.
   */
  Node* data() { return m_data; }

  /**
   * \brief Returns a pointer to the beginning of unpadded data.
   */
  const Node* data() const { return m_data; }

  /**
   * \brief Returns a pointer to the beginning of padded data.
   */
  Node* paddedData() { return m_storage.get(); }

  /**
   * \brief Returns a pointer to the beginning of padded data.
   */
  const Node* paddedData() const { return m_storage.get(); }

  /**
   * Returns the number of nodes in a row, including padding nodes.
   */
  int stride() const { return m_stride; }

  /**
   * Returns the number of nodes in a row, excluding padding nodes.
   */
  int width() const { return m_width; }

  /**
   * Returns the number of nodes in a column, excluding padding nodes.
   */
  int height() const { return m_height; }

  /**
   * Returns the number of padding layers from each side.
   */
  int padding() const { return m_padding; }

  void swap(Grid& other);

 private:
  template <typename T>
  static void basicSwap(T& o1, T& o2) {
    // Just to avoid incoduing the heavy <algorithm> header.
    T tmp(o1);
    o1 = o2;
    o2 = tmp;
  }

  boost::scoped_array<Node> m_storage;
  Node* m_data;
  int m_width;
  int m_height;
  int m_stride;
  int m_padding;
};


template <typename Node>
Grid<Node>::Grid() : m_data(0), m_width(0), m_height(0), m_stride(0), m_padding(0) {}

template <typename Node>
Grid<Node>::Grid(int width, int height, int padding)
    : m_storage(new Node[(width + padding * 2) * (height + padding * 2)]),
      m_data(m_storage.get() + (width + padding * 2) * padding + padding),
      m_width(width),
      m_height(height),
      m_stride(width + padding * 2),
      m_padding(padding) {}

template <typename Node>
Grid<Node>::Grid(const Grid& other)
    : m_storage(new Node[(other.stride() * (other.height() + other.padding() * 2))]),
      m_data(m_storage.get() + other.stride() * other.padding() + other.padding()),
      m_width(other.width()),
      m_height(other.height()),
      m_stride(other.stride()),
      m_padding(other.padding()) {
  const int len = m_stride * (m_height + m_padding * 2);
  for (int i = 0; i < len; ++i) {
    m_storage[i] = other.m_storage[i];
  }
}

template <typename Node>
void Grid<Node>::initPadding(const Node& paddingNode) {
  if (m_padding == 0) {
    // No padding.
    return;
  }

  Node* line = m_storage.get();
  for (int row = 0; row < m_padding; ++row) {
    for (int x = 0; x < m_stride; ++x) {
      line[x] = paddingNode;
    }
    line += m_stride;
  }

  for (int y = 0; y < m_height; ++y) {
    for (int col = 0; col < m_padding; ++col) {
      line[col] = paddingNode;
    }
    for (int col = m_stride - m_padding; col < m_stride; ++col) {
      line[col] = paddingNode;
    }
    line += m_stride;
  }

  for (int row = 0; row < m_padding; ++row) {
    for (int x = 0; x < m_stride; ++x) {
      line[x] = paddingNode;
    }
    line += m_stride;
  }
}  // >::initPadding

template <typename Node>
void Grid<Node>::initInterior(const Node& interiorNode) {
  Node* line = m_data;
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      line[x] = interiorNode;
    }
    line += m_stride;
  }
}

template <typename Node>
void Grid<Node>::swap(Grid& other) {
  m_storage.swap(other.m_storage);
  basicSwap(m_data, other.m_data);
  basicSwap(m_width, other.m_width);
  basicSwap(m_height, other.m_height);
  basicSwap(m_stride, other.m_stride);
  basicSwap(m_padding, other.m_padding);
}

template <typename Node>
void swap(Grid<Node>& o1, Grid<Node>& o2) {
  o1.swap(o2);
}

#endif  // ifndef SCANTAILOR_FOUNDATION_GRID_H_
