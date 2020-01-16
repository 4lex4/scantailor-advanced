// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ContentSpanFinder.h>
#include <SlicedHistogram.h>
#include <Span.h>

#include <boost/test/unit_test.hpp>
#include <vector>

namespace Tests {
using namespace imageproc;

BOOST_AUTO_TEST_SUITE(ContentSpanFinderTestSuite)

BOOST_AUTO_TEST_CASE(test_empty_input) {
  ContentSpanFinder spanFinder;

  std::vector<Span> spans;
  spanFinder.find(SlicedHistogram(), [&](const Span& span) { spans.push_back(span); });

  BOOST_CHECK(spans.empty());
}

BOOST_AUTO_TEST_CASE(test_min_content_width) {
  SlicedHistogram hist;
  hist.setSize(9);
  hist[0] = 0;
  hist[1] = 1;
  hist[2] = 0;
  hist[3] = 1;
  hist[4] = 1;
  hist[5] = 1;
  hist[6] = 0;
  hist[7] = 1;
  hist[8] = 1;

  ContentSpanFinder spanFinder;
  spanFinder.setMinContentWidth(2);

  std::vector<Span> spans;
  spanFinder.find(hist, [&](const Span& span) { spans.push_back(span); });

  BOOST_REQUIRE(spans.size() == 2);
  BOOST_REQUIRE(spans[0] == Span(3, 3 + 3));
  BOOST_REQUIRE(spans[1] == Span(7, 7 + 2));
}

BOOST_AUTO_TEST_CASE(test_min_whitespace_width) {
  SlicedHistogram hist;
  hist.setSize(9);
  hist[0] = 0;
  hist[1] = 1;
  hist[2] = 0;
  hist[3] = 1;
  hist[4] = 1;
  hist[5] = 0;
  hist[6] = 0;
  hist[7] = 1;
  hist[8] = 1;

  ContentSpanFinder spanFinder;
  spanFinder.setMinWhitespaceWidth(2);

  std::vector<Span> spans;
  spanFinder.find(hist, [&](const Span& span) { spans.push_back(span); });

  BOOST_REQUIRE(spans.size() == 2);
  BOOST_REQUIRE(spans[0] == Span(1, 1 + 4));
  BOOST_REQUIRE(spans[1] == Span(7, 7 + 2));
}

BOOST_AUTO_TEST_CASE(test_min_content_and_whitespace_width) {
  SlicedHistogram hist;
  hist.setSize(9);
  hist[0] = 0;
  hist[1] = 1;
  hist[2] = 0;
  hist[3] = 1;
  hist[4] = 1;
  hist[5] = 0;
  hist[6] = 0;
  hist[7] = 1;
  hist[8] = 0;

  ContentSpanFinder spanFinder;
  spanFinder.setMinContentWidth(2);
  spanFinder.setMinWhitespaceWidth(2);

  std::vector<Span> spans;
  spanFinder.find(hist, [&](const Span& span) { spans.push_back(span); });

  // Note that although a content block at index 1 is too short,
  // it's still allowed to merge with content at positions 3 and 4
  // because the whitespace between them is too short as well.

  BOOST_REQUIRE(spans.size() == 1);
  BOOST_REQUIRE(spans[0] == Span(1, 1 + 4));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests