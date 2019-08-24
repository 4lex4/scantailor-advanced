// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "EstimateBackground.h"
#include <BinaryImage.h>
#include <BitOps.h>
#include <Connectivity.h>
#include <GrayImage.h>
#include <GrayRasterOp.h>
#include <Morphology.h>
#include <PolygonRasterizer.h>
#include <PolynomialLine.h>
#include <PolynomialSurface.h>
#include <RasterOpGeneric.h>
#include <Scale.h>
#include <SeedFill.h>
#include <Transform.h>
#include <QDebug>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/control_structures.hpp>
#include <boost/lambda/lambda.hpp>
#include "DebugImages.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"

using namespace imageproc;

struct AbsoluteDifference {
  static uint8_t transform(uint8_t src, uint8_t dst) { return static_cast<uint8_t>(std::abs(int(src) - int(dst))); }
};

/**
 * The same as seedFillGrayInPlace() with a seed of two black lines
 * at top and bottom, except here colors may only spread vertically.
 */
static void seedFillTopBottomInPlace(GrayImage& image) {
  uint8_t* const data = image.data();
  const int stride = image.stride();

  const int width = image.width();
  const int height = image.height();

  std::vector<uint8_t> seedLine(height, 0xff);

  for (int x = 0; x < width; ++x) {
    uint8_t* p = data + x;

    uint8_t prev = 0;  // black
    for (int y = 0; y < height; ++y) {
      seedLine[y] = prev = std::max(*p, prev);
      p += stride;
    }

    prev = 0;  // black
    for (int y = height - 1; y >= 0; --y) {
      p -= stride;
      *p = prev = std::max(*p, std::min(seedLine[y], prev));
    }
  }
}

static void morphologicalPreprocessingInPlace(GrayImage& image, DebugImages* dbg) {
  using namespace boost::lambda;

  // We do morphological preprocessing with one of two methods.  The first
  // one is good for cases when the dark area is in the middle of the image,
  // touching at least one of the vertical edges and not touching the horizontal one.
  // The second method is good for pages that have pictures (partly) in the
  // shadow of the spine.  Most of the other cases can be handled by any of these
  // two methods.

  GrayImage method1(createFramedImage(image.size()));
  seedFillGrayInPlace(method1, image, CONN8);

  // This will get rid of the remnants of letters.  Note that since we know we
  // are working with at most 300x300 px images, we can just hardcode the size.
  method1 = openGray(method1, QSize(1, 20), 0x00);
  if (dbg) {
    dbg->add(method1, "preproc_method1");
  }

  seedFillTopBottomInPlace(image);
  if (dbg) {
    dbg->add(image, "preproc_method2");
  }

  // Now let's estimate, which of the methods is better for this case.

  // Take the difference between two methods.
  GrayImage diff(image);
  rasterOpGeneric(diff.data(), diff.stride(), diff.size(), method1.data(), method1.stride(), _1 -= _2);
  if (dbg) {
    dbg->add(diff, "raw_diff");
  }

  // Approximate the difference using a polynomial function.
  // If it fits well into our data set, we consider the difference
  // to be caused by a shadow rather than a picture, and use method1.
  GrayImage approximated(PolynomialSurface(3, 3, diff).render(diff.size()));
  if (dbg) {
    dbg->add(approximated, "approx_diff");
  }
  // Now let's take the difference between the original difference
  // and approximated difference.
  rasterOpGeneric(diff.data(), diff.stride(), diff.size(), approximated.data(), approximated.stride(),
                  if_then_else(_1 > _2, _1 -= _2, _1 = _2 - _1));
  approximated = GrayImage();  // save memory.
  if (dbg) {
    dbg->add(diff, "raw_vs_approx_diff");
  }

  // Our final decision is like this:
  // If we have at least 1% of pixels that are greater than 10,
  // we consider that we have a picture rather than a shadow,
  // and use method2.

  int sum = 0;
  GrayscaleHistogram hist(diff);
  for (int i = 255; i > 10; --i) {
    sum += hist[i];
  }

  // qDebug() << "% of pixels > 10: " << 100.0 * sum / (diff.width() * diff.height());

  if (sum < 0.01 * (diff.width() * diff.height())) {
    image = method1;
    if (dbg) {
      dbg->add(image, "use_method1");
    }
  } else {
    // image is already set to method2
    if (dbg) {
      dbg->add(image, "use_method2");
    }
  }
}  // morphologicalPreprocessingInPlace

imageproc::PolynomialSurface estimateBackground(const GrayImage& input,
                                                const QPolygonF& areaToConsider,
                                                const TaskStatus& status,
                                                DebugImages* dbg) {
  QSize reducedSize(input.size());
  reducedSize.scale(300, 300, Qt::KeepAspectRatio);
  GrayImage background(scaleToGray(GrayImage(input), reducedSize));
  if (dbg) {
    dbg->add(background, "downscaled");
  }

  status.throwIfCancelled();

  morphologicalPreprocessingInPlace(background, dbg);

  status.throwIfCancelled();

  const int width = background.width();
  const int height = background.height();

  const uint8_t* const bgData = background.data();
  const int bgStride = background.stride();

  BinaryImage mask(background.size(), BLACK);

  if (!areaToConsider.isEmpty()) {
    QTransform xform;
    xform.scale((double) reducedSize.width() / input.width(), (double) reducedSize.height() / input.height());
    PolygonRasterizer::fillExcept(mask, WHITE, xform.map(areaToConsider), Qt::WindingFill);
  }

  if (dbg) {
    dbg->add(mask, "areaToConsider");
  }

  uint32_t* maskData = mask.data();
  int maskStride = mask.wordsPerLine();

  std::vector<uint8_t> line(std::max(width, height), 0);
  const uint32_t msb = uint32_t(1) << 31;

  status.throwIfCancelled();

  // Smooth every horizontal line with a polynomial,
  // then mask pixels that became significantly lighter.
  for (int x = 0; x < width; ++x) {
    const uint32_t mask = ~(msb >> (x & 31));

    const int degree = 2;
    PolynomialLine pl(degree, bgData + x, height, bgStride);
    pl.output(&line[0], height, 1);

    const uint8_t* pBg = bgData + x;
    uint32_t* pMask = maskData + (x >> 5);
    for (int y = 0; y < height; ++y) {
      if (*pBg + 30 < line[y]) {
        *pMask &= mask;
      }
      pBg += bgStride;
      pMask += maskStride;
    }
  }

  status.throwIfCancelled();
  // Smooth every vertical line with a polynomial,
  // then mask pixels that became significantly lighter.
  const uint8_t* bgLine = bgData;
  uint32_t* maskLine = maskData;
  for (int y = 0; y < height; ++y) {
    const int degree = 4;
    PolynomialLine pl(degree, bgLine, width, 1);
    pl.output(&line[0], width, 1);

    for (int x = 0; x < width; ++x) {
      if (bgLine[x] + 30 < line[x]) {
        maskLine[x >> 5] &= ~(msb >> (x & 31));
      }
    }

    bgLine += bgStride;
    maskLine += maskStride;
  }

  if (dbg) {
    dbg->add(mask, "mask");
  }

  status.throwIfCancelled();

  mask = erodeBrick(mask, QSize(3, 3));
  if (dbg) {
    dbg->add(mask, "eroded");
  }

  status.throwIfCancelled();

  // Update those because mask was overwritten.
  maskData = mask.data();
  maskStride = mask.wordsPerLine();
  // Check each horizontal line.  If it's mostly
  // white (ignored), then make it completely white.
  const int lastWordIdx = (width - 1) >> 5;
  const uint32_t lastWordMask = ~uint32_t(0) << (32 - width - (lastWordIdx << 5));
  maskLine = maskData;
  for (int y = 0; y < height; ++y, maskLine += maskStride) {
    int blackCount = 0;
    int i = 0;

    // Complete words.
    for (; i < lastWordIdx; ++i) {
      blackCount += countNonZeroBits(maskLine[i]);
    }

    // The last (possible incomplete) word.
    blackCount += countNonZeroBits(maskLine[i] & lastWordMask);

    if (blackCount < width / 4) {
      memset(maskLine, 0, (lastWordIdx + 1) * sizeof(*maskLine));
    }
  }

  status.throwIfCancelled();
  // Check each vertical line.  If it's mostly
  // white (ignored), then make it completely white.
  for (int x = 0; x < width; ++x) {
    const uint32_t mask = msb >> (x & 31);
    uint32_t* pMask = maskData + (x >> 5);
    int blackCount = 0;
    for (int y = 0; y < height; ++y) {
      if (*pMask & mask) {
        ++blackCount;
      }
      pMask += maskStride;
    }
    if (blackCount < height / 4) {
      for (int y = height - 1; y >= 0; --y) {
        pMask -= maskStride;
        *pMask &= ~mask;
      }
    }
  }

  if (dbg) {
    dbg->add(mask, "lines_extended");
  }

  status.throwIfCancelled();

  return PolynomialSurface(8, 5, background, mask);
}  // estimateBackground
