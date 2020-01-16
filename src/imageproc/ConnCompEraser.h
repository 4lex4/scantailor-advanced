// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_CONNCOMPERASER_H_
#define SCANTAILOR_IMAGEPROC_CONNCOMPERASER_H_

#include <cstdint>
#include <stack>

#include "BinaryImage.h"
#include "ConnComp.h"
#include "Connectivity.h"
#include "NonCopyable.h"

namespace imageproc {
class ConnComp;

/**
 * \brief Erases connected components one by one and returns their bounding boxes.
 */
class ConnCompEraser {
  DECLARE_NON_COPYABLE(ConnCompEraser)

 public:
  /**
   * \brief Constructor.
   *
   * \param image The image from which connected components are to be erased.
   *        If you don't need the original image, pass image.release(), to
   *        avoid unnecessary copy-on-write.
   * \param conn Defines which neighbouring pixels are considered to be connected.
   */
  ConnCompEraser(const BinaryImage& image, Connectivity conn);

  /**
   * \brief Erase the next connected component and return its bounding box.
   *
   * If there are no black pixels remaining, returns a null ConnComp.
   */
  ConnComp nextConnComp();

  /**
   * \brief Returns the image in its present state.
   *
   * Every time nextConnComp() is called, a connected component
   * is erased from the image, assuming there was one.
   */
  const BinaryImage& image() const { return m_image; }

 private:
  struct Segment {
    uint32_t* line;

    /**< Pointer to the beginning of the line. */
    int xleft;

    /**< Leftmost pixel to process. */
    int xright;

    /**< Rightmost pixel to process. */
    int y;

    /**< y value of the line to be processed. */
    int dy;

    /**< Vertical direction: 1 or -1. */
    int dyWpl; /**< words_per_line or -words_per_line. */
  };

  struct BBox;

  void pushSegSameDir(const Segment& seg, int xleft, int xright, BBox& bbox);

  void pushSegInvDir(const Segment& seg, int xleft, int xright, BBox& bbox);

  void pushInitialSegments();

  bool moveToNextBlackPixel();

  ConnComp eraseConnComp4();

  ConnComp eraseConnComp8();

  static uint32_t getBit(const uint32_t* line, int x);

  static void clearBit(uint32_t* line, int x);

  BinaryImage m_image;
  uint32_t* m_line;
  const int m_width;
  const int m_height;
  const int m_wpl;
  const Connectivity m_connectivity;
  std::stack<Segment> m_segStack;
  int m_x;
  int m_y;
};
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_CONNCOMPERASER_H_
