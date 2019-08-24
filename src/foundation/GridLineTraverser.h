// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_GRIDLINETRAVERSER_H_
#define SCANTAILOR_FOUNDATION_GRIDLINETRAVERSER_H_

#include <QLineF>

/**
 * \brief Traverses a grid along a line segment.
 *
 * Think about drawing a line on an image.
 */
class GridLineTraverser {
  // Member-wise copying is OK.
 public:
  explicit GridLineTraverser(const QLineF& line);

  bool hasNext() const { return m_stopsDone < m_totalStops; }

  QPoint next();

 private:
  QLineF m_line;
  double m_dt;
  int m_totalStops;
  int m_stopsDone;
};


#endif
