// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef CONTENTSPANFINDER_H_
#define CONTENTSPANFINDER_H_

#include "Span.h"
#include "VirtualFunction.h"

namespace imageproc {
class SlicedHistogram;
}

class ContentSpanFinder {
  // Member-wise copying is OK.
 public:
  ContentSpanFinder() : m_minContentWidth(1), m_minWhitespaceWidth(1) {}

  void setMinContentWidth(int value) { m_minContentWidth = value; }

  void setMinWhitespaceWidth(int value) { m_minWhitespaceWidth = value; }

  /**
   * \brief Find content spans.
   *
   * Note that content blocks shorter than min-content-width are still
   * allowed to merge with other content blocks, if whitespace that
   * separates them is shorter than min-whitespace-width.
   */
  template <typename T>
  void find(const imageproc::SlicedHistogram& histogram, T handler) const;

 private:
  void findImpl(const imageproc::SlicedHistogram& histogram, const VirtualFunction<void, const Span&>& handler) const;

  int m_minContentWidth;
  int m_minWhitespaceWidth;
};


template <typename Callable>
void ContentSpanFinder::find(const imageproc::SlicedHistogram& histogram, Callable handler) const {
  findImpl(histogram, ProxyFunction<Callable, void, const Span&>(handler));
}

#endif  // ifndef CONTENTSPANFINDER_H_
