// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SkewFinder.h"
#include <QDebug>
#include <cmath>
#include "BinaryImage.h"
#include "BitOps.h"
#include "Constants.h"
#include "ReduceThreshold.h"
#include "Shear.h"

namespace imageproc {
const double Skew::GOOD_CONFIDENCE = 2.0;

const double SkewFinder::DEFAULT_MAX_ANGLE = 7.0;

const double SkewFinder::DEFAULT_MIN_ANGLE = 0.1;

const double SkewFinder::DEFAULT_ACCURACY = 0.1;

const int SkewFinder::DEFAULT_COARSE_REDUCTION = 2;

const int SkewFinder::DEFAULT_FINE_REDUCTION = 1;

const double SkewFinder::LOW_SCORE = 1000.0;

SkewFinder::SkewFinder()
    : m_maxAngle(DEFAULT_MAX_ANGLE),
      m_minAngle(DEFAULT_MIN_ANGLE),
      m_accuracy(DEFAULT_ACCURACY),
      m_resolutionRatio(1.0),
      m_coarseReduction(DEFAULT_COARSE_REDUCTION),
      m_fineReduction(DEFAULT_FINE_REDUCTION) {}

void SkewFinder::setMaxAngle(const double maxAngle) {
  if ((maxAngle < 0.0) || (maxAngle > 45.0)) {
    throw std::invalid_argument("SkewFinder: max skew angle is invalid");
  }
  m_maxAngle = maxAngle;
}

void SkewFinder::setMinAngle(double minAngle) {
  if ((minAngle < 0.0) || (minAngle > m_maxAngle)) {
    throw std::invalid_argument("SkewFinder: min skew angle is invalid");
  }
  m_minAngle = minAngle;
}

void SkewFinder::setDesiredAccuracy(const double accuracy) {
  m_accuracy = accuracy;
}

void SkewFinder::setCoarseReduction(const int reduction) {
  if (reduction < 0) {
    throw std::invalid_argument("SkewFinder: coarse reduction is invalid");
  }
  m_coarseReduction = reduction;
}

void SkewFinder::setFineReduction(const int reduction) {
  if (reduction < 0) {
    throw std::invalid_argument("SkewFinder: fine reduction is invalid");
  }
  m_fineReduction = reduction;
}

void SkewFinder::setResolutionRatio(const double ratio) {
  if (ratio <= 0.0) {
    throw std::invalid_argument("SkewFinder: resolution ratio is invalid");
  }
  m_resolutionRatio = ratio;
}

Skew SkewFinder::findSkew(const BinaryImage& image) const {
  if (image.isNull()) {
    throw std::invalid_argument("SkewFinder: null image was provided");
  }

  ReduceThreshold coarseReduced(image);
  const int minReduction = std::min(m_coarseReduction, m_fineReduction);
  for (int i = 0; i < minReduction; ++i) {
    coarseReduced.reduce(i == 0 ? 1 : 2);
  }

  ReduceThreshold fineReduced(coarseReduced.image());

  for (int i = minReduction; i < m_coarseReduction; ++i) {
    coarseReduced.reduce(i == 0 ? 1 : 2);
  }

  BinaryImage skewed(coarseReduced.image().size());
  const double coarseStep = 1.0;  // degrees
  // Coarse linear search.
  int numCoarseScores = 0;
  double sumCoarseScores = 0.0;
  double bestCoarseScore = 0.0;
  double bestCoarseAngle = -m_maxAngle;
  for (double angle = -m_maxAngle; angle <= m_maxAngle; angle += coarseStep) {
    const double score = process(coarseReduced, skewed, angle);
    sumCoarseScores += score;
    ++numCoarseScores;
    if (score > bestCoarseScore) {
      bestCoarseAngle = angle;
      bestCoarseScore = score;
    }
  }

  if (m_accuracy >= coarseStep) {
    double confidence = 0.0;
    if (numCoarseScores > 1) {
      confidence = bestCoarseScore / sumCoarseScores * numCoarseScores;
    }

    return Skew(-bestCoarseAngle, confidence - 1.0);
  }

  for (int i = minReduction; i < m_fineReduction; ++i) {
    fineReduced.reduce(i == 0 ? 1 : 2);
  }

  if (m_coarseReduction != m_fineReduction) {
    skewed = BinaryImage(fineReduced.image().size());
  }
  // Fine binary search.
  double anglePlus = bestCoarseAngle + 0.5 * coarseStep;
  double angleMinus = bestCoarseAngle - 0.5 * coarseStep;
  double scorePlus = process(fineReduced, skewed, anglePlus);
  double scoreMinus = process(fineReduced, skewed, angleMinus);
  const double fineScore1 = scorePlus;
  const double fineScore2 = scoreMinus;
  while (anglePlus - angleMinus > m_accuracy) {
    if (scorePlus > scoreMinus) {
      angleMinus = 0.5 * (anglePlus + angleMinus);
      scoreMinus = process(fineReduced, skewed, angleMinus);
    } else if (scorePlus < scoreMinus) {
      anglePlus = 0.5 * (anglePlus + angleMinus);
      scorePlus = process(fineReduced, skewed, anglePlus);
    } else {
      // This protects us from unreasonably low m_accuracy.
      break;
    }
  }

  double bestAngle;
  double bestScore;
  if (scorePlus > scoreMinus) {
    bestAngle = anglePlus;
    bestScore = scorePlus;
  } else {
    bestAngle = angleMinus;
    bestScore = scoreMinus;
  }

  if (std::abs(bestAngle) < m_minAngle) {
    return Skew(0.0, Skew::GOOD_CONFIDENCE);
  }
  if (bestScore <= LOW_SCORE) {
    return Skew(-bestAngle, 0.0);  // Zero confidence.
  }

  double confidence;
  if (numCoarseScores > 1) {
    confidence = bestScore / sumCoarseScores * numCoarseScores;
  } else {
    int numScores = numCoarseScores;
    double sumScores = sumCoarseScores;
    numScores += 2;
    sumScores += fineScore1;
    sumScores += fineScore2;
    confidence = bestScore / sumScores * numScores;
  }

  return Skew(-bestAngle, confidence - 1.0);
}  // SkewFinder::findSkew

double SkewFinder::process(const BinaryImage& src, BinaryImage& dst, const double angle) const {
  const double tg = std::tan(angle * constants::DEG2RAD);
  const double xCenter = 0.5 * dst.width();
  vShearFromTo(src, dst, tg / m_resolutionRatio, xCenter, WHITE);

  return calcScore(dst);
}

double SkewFinder::calcScore(const BinaryImage& image) {
  const int width = image.width();
  const int height = image.height();
  const uint32_t* line = image.data();
  const int wpl = image.wordsPerLine();
  const int lastWordIdx = (width - 1) >> 5;
  const uint32_t lastWordMask = ~uint32_t(0) << (31 - ((width - 1) & 31));

  double score = 0.0;
  int lastLineBlackPixels = 0;
  for (int y = 0; y < height; ++y, line += wpl) {
    int numBlackPixels = 0;
    int i = 0;
    for (; i != lastWordIdx; ++i) {
      numBlackPixels += countNonZeroBits(line[i]);
    }
    numBlackPixels += countNonZeroBits(line[i] & lastWordMask);

    if (y != 0) {
      const double diff = numBlackPixels - lastLineBlackPixels;
      score += diff * diff;
    }
    lastLineBlackPixels = numBlackPixels;
  }

  return score;
}
}  // namespace imageproc