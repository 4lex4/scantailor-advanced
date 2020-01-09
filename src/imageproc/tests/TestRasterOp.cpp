// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BinaryImage.h>
#include <RasterOp.h>
#include <QImage>
#include <boost/test/unit_test.hpp>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(RasterOpTestSuite)

template <typename Rop>
static bool checkSubimageRop(const QImage& dst, const QRect& dstRect, const QImage& src, const QPoint& srcPt) {
  BinaryImage dstBi(dst);
  const BinaryImage srcBi(src);
  rasterOp<Rop>(dstBi, dstRect, srcBi, srcPt);
  // Here we assume that full-image raster operations work correctly.
  BinaryImage dstSubimage(dst.copy(dstRect));
  const QRect srcRect(srcPt, dstRect.size());
  const BinaryImage srcSubimage(src.copy(srcRect));
  rasterOp<Rop>(dstSubimage, dstSubimage.rect(), srcSubimage, QPoint(0, 0));

  return dstSubimage.toQImage() == dstBi.toQImage().copy(dstRect);
}

BOOST_AUTO_TEST_CASE(test_small_image) {
  static const int inp[]
      = {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0,
         0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0};

  static const int mask[]
      = {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
         0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0};

  static const int out[]
      = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
         0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0};

  BinaryImage img(makeBinaryImage(inp, 9, 8));
  const BinaryImage maskImg(makeBinaryImage(mask, 9, 8));

  using Rop = RopAnd<RopDst, RopSrc>;

  rasterOp<Rop>(img, img.rect(), maskImg, QPoint(0, 0));
  BOOST_CHECK(img == makeBinaryImage(out, 9, 8));
}

namespace {
class Tester1 {
 public:
  Tester1();

  bool testFullImage() const;

  bool testSubImage(const QRect& dstRect, const QPoint& srcPt) const;

 private:
  using Rop = RopXor<RopDst, RopSrc>;

  BinaryImage m_src;
  BinaryImage m_dstBefore;
  BinaryImage m_dstAfter;
};


Tester1::Tester1() {
  const int w = 400;
  const int h = 300;

  std::vector<int> src(w * h);
  for (int& i : src) {
    i = rand() & 1;
  }

  std::vector<int> dst(w * h);
  for (int& i : dst) {
    i = rand() & 1;
  }

  std::vector<int> res(w * h);
  for (size_t i = 0; i < res.size(); ++i) {
    res[i] = Rop::transform(src[i], dst[i]) & 1;
  }

  m_src = makeBinaryImage(&src[0], w, h);
  m_dstBefore = makeBinaryImage(&dst[0], w, h);
  m_dstAfter = makeBinaryImage(&res[0], w, h);
}

bool Tester1::testFullImage() const {
  BinaryImage dst(m_dstBefore);
  rasterOp<Rop>(dst, dst.rect(), m_src, QPoint(0, 0));

  return dst == m_dstAfter;
}

bool Tester1::testSubImage(const QRect& dstRect, const QPoint& srcPt) const {
  const QImage dstBefore(m_dstBefore.toQImage());
  const QImage& dst(dstBefore);
  const QImage src(m_src.toQImage());

  if (!checkSubimageRop<Rop>(dst, dstRect, src, srcPt)) {
    return false;
  }

  return surroundingsIntact(dst, dstBefore, dstRect);
}
}  // namespace
BOOST_AUTO_TEST_CASE(test_large_image) {
  Tester1 tester;
  BOOST_REQUIRE(tester.testFullImage());
  BOOST_REQUIRE(tester.testSubImage(QRect(101, 32, 211, 151), QPoint(101, 41)));
  BOOST_REQUIRE(tester.testSubImage(QRect(101, 32, 211, 151), QPoint(99, 99)));
  BOOST_REQUIRE(tester.testSubImage(QRect(101, 32, 211, 151), QPoint(104, 64)));
}

namespace {
class Tester2 {
 public:
  Tester2();

  bool testBlockMove(const QRect& rect, int dx, int dy);

 private:
  BinaryImage m_image;
};


Tester2::Tester2() {
  m_image = randomBinaryImage(400, 300);
}

bool Tester2::testBlockMove(const QRect& rect, const int dx, const int dy) {
  BinaryImage dst(m_image);
  const QRect dstRect(rect.translated(dx, dy));
  rasterOp<RopSrc>(dst, dstRect, dst, rect.topLeft());
  QImage qSrc(m_image.toQImage());
  QImage qDst(dst.toQImage());
  if (qSrc.copy(rect) != qDst.copy(dstRect)) {
    return false;
  }

  return surroundingsIntact(qDst, qSrc, dstRect);
}
}  // namespace
BOOST_AUTO_TEST_CASE(test_move_blocks) {
  Tester2 tester;
  BOOST_REQUIRE(tester.testBlockMove(QRect(0, 0, 97, 150), 1, 0));
  BOOST_REQUIRE(tester.testBlockMove(QRect(100, 50, 15, 100), -1, 0));
  BOOST_REQUIRE(tester.testBlockMove(QRect(200, 200, 200, 100), -1, -1));
  BOOST_REQUIRE(tester.testBlockMove(QRect(51, 35, 199, 200), 0, 1));
  BOOST_REQUIRE(tester.testBlockMove(QRect(51, 35, 199, 200), 1, 1));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc