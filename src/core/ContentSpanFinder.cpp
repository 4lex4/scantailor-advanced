// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ContentSpanFinder.h"

#include <SlicedHistogram.h>

using namespace imageproc;

void ContentSpanFinder::findImpl(const SlicedHistogram& histogram,
                                 const VirtualFunction<void, const Span&>& handler) const {
  const auto histSize = static_cast<int>(histogram.size());

  int i = 0;
  int contentEnd = -m_minWhitespaceWidth;
  int contentBegin = contentEnd;

  while (true) {
    // Find the next content position.
    for (; i < histSize; ++i) {
      if (histogram[i] != 0) {
        break;
      }
    }

    if (i - contentEnd >= m_minWhitespaceWidth) {
      // Whitespace is long enough to break the content block.

      // Note that contentEnd is initialized to
      // -m_minWhitespaceWidth to make this test
      // pass on the first content block, in order to avoid
      // growing a non existing content block.

      if (contentEnd - contentBegin >= m_minContentWidth) {
        handler(Span(contentBegin, contentEnd));
      }

      contentBegin = i;
    }

    if (i == histSize) {
      break;
    }

    // Find the next whitespace position.
    for (; i < histSize; ++i) {
      if (histogram[i] == 0) {
        break;
      }
    }
    contentEnd = i;
  }

  if (contentEnd - contentBegin >= m_minContentWidth) {
    handler(Span(contentBegin, contentEnd));
  }
}  // ContentSpanFinder::findImpl
