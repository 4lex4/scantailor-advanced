// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ContentSpanFinder.h"
#include <SlicedHistogram.h>

using namespace imageproc;

void ContentSpanFinder::findImpl(const SlicedHistogram& histogram,
                                 const VirtualFunction<void, const Span&>& handler) const {
  const auto hist_size = static_cast<int>(histogram.size());

  int i = 0;
  int content_end = -m_minWhitespaceWidth;
  int content_begin = content_end;

  while (true) {
    // Find the next content position.
    for (; i < hist_size; ++i) {
      if (histogram[i] != 0) {
        break;
      }
    }

    if (i - content_end >= m_minWhitespaceWidth) {
      // Whitespace is long enough to break the content block.

      // Note that content_end is initialized to
      // -m_minWhitespaceWidth to make this test
      // pass on the first content block, in order to avoid
      // growing a non existing content block.

      if (content_end - content_begin >= m_minContentWidth) {
        handler(Span(content_begin, content_end));
      }

      content_begin = i;
    }

    if (i == hist_size) {
      break;
    }

    // Find the next whitespace position.
    for (; i < hist_size; ++i) {
      if (histogram[i] == 0) {
        break;
      }
    }
    content_end = i;
  }

  if (content_end - content_begin >= m_minContentWidth) {
    handler(Span(content_begin, content_end));
  }
}  // ContentSpanFinder::findImpl
