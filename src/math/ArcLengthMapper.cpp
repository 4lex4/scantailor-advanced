// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ArcLengthMapper.h"
#include <cassert>
#include <cmath>

ArcLengthMapper::Hint::Hint() : m_lastSegment(0), m_direction(1) {}

void ArcLengthMapper::Hint::update(int newSegment) {
  m_direction = newSegment < m_lastSegment ? -1 : 1;
  m_lastSegment = newSegment;
}

ArcLengthMapper::ArcLengthMapper() : m_prevFX() {}

void ArcLengthMapper::addSample(double x, double fx) {
  double arcLen = 0;

  if (!m_samples.empty()) {
    const double dx = x - m_samples.back().x;
    const double dy = fx - m_prevFX;
    assert(dx > 0);
    arcLen = m_samples.back().arcLen + std::sqrt(dx * dx + dy * dy);
  }

  m_samples.emplace_back(x, arcLen);
  m_prevFX = fx;
}

double ArcLengthMapper::totalArcLength() const {
  return m_samples.size() < 2 ? 0.0 : m_samples.back().arcLen;
}

void ArcLengthMapper::normalizeRange(double totalArcLen) {
  if (m_samples.size() <= 1) {
    // If size == 1, samples.back().arcLen below will be 0.
    return;
  }

  assert(totalArcLen != 0);

  const double scale = totalArcLen / m_samples.back().arcLen;
  for (Sample& sample : m_samples) {
    sample.arcLen *= scale;
  }
}

double ArcLengthMapper::arcLenToX(double arcLen, Hint& hint) const {
  switch (m_samples.size()) {
    case 0:
      return 0;
    case 1:
      return m_samples.front().x;
  }

  if (arcLen < 0) {
    // Beyond the first sample.
    hint.update(0);

    return interpolateArcLenInSegment(arcLen, 0);
  } else if (arcLen > m_samples.back().arcLen) {
    // Beyond the last sample.
    hint.update(static_cast<int>(m_samples.size() - 2));

    return interpolateArcLenInSegment(arcLen, hint.m_lastSegment);
  }

  // Check in the answer is in the segment provided by hint,
  // or in an adjacent one.
  if (checkSegmentForArcLen(arcLen, hint.m_lastSegment)) {
    return interpolateArcLenInSegment(arcLen, hint.m_lastSegment);
  } else if (checkSegmentForArcLen(arcLen, hint.m_lastSegment + hint.m_direction)) {
    hint.update(hint.m_lastSegment + hint.m_direction);

    return interpolateArcLenInSegment(arcLen, hint.m_lastSegment);
  } else if (checkSegmentForArcLen(arcLen, hint.m_lastSegment - hint.m_direction)) {
    hint.update(hint.m_lastSegment - hint.m_direction);

    return interpolateArcLenInSegment(arcLen, hint.m_lastSegment);
  }
  // Do a binary search.
  int leftIdx = 0;
  auto rightIdx = static_cast<int>(m_samples.size() - 1);
  double leftArcLen = m_samples[leftIdx].arcLen;
  while (leftIdx + 1 < rightIdx) {
    const int midIdx = (leftIdx + rightIdx) >> 1;
    const double midArcLen = m_samples[midIdx].arcLen;
    if ((arcLen - midArcLen) * (arcLen - leftArcLen) <= 0) {
      // Note: <= 0 vs < 0 is actually important for this branch.
      // 0 would indicate either left or mid point is our exact answer.
      rightIdx = midIdx;
    } else {
      leftIdx = midIdx;
      leftArcLen = midArcLen;
    }
  }

  hint.update(leftIdx);

  return interpolateArcLenInSegment(arcLen, leftIdx);
}  // ArcLengthMapper::arcLenToX

double ArcLengthMapper::xToArcLen(double x, Hint& hint) const {
  switch (m_samples.size()) {
    case 0:
      return 0;
    case 1:
      return m_samples.front().arcLen;
  }

  if (x < m_samples.front().x) {
    // Beyond the first sample.
    hint.update(0);

    return interpolateXInSegment(x, 0);
  } else if (x > m_samples.back().x) {
    // Beyond the last sample.
    hint.update(static_cast<int>(m_samples.size() - 2));

    return interpolateXInSegment(x, hint.m_lastSegment);
  }

  // Check in the answer is in the segment provided by hint,
  // or in an adjacent one.
  if (checkSegmentForX(x, hint.m_lastSegment)) {
    return interpolateXInSegment(x, hint.m_lastSegment);
  } else if (checkSegmentForX(x, hint.m_lastSegment + hint.m_direction)) {
    hint.update(hint.m_lastSegment + hint.m_direction);

    return interpolateXInSegment(x, hint.m_lastSegment);
  } else if (checkSegmentForX(x, hint.m_lastSegment - hint.m_direction)) {
    hint.update(hint.m_lastSegment - hint.m_direction);

    return interpolateXInSegment(x, hint.m_lastSegment);
  }
  // Do a binary search.
  int leftIdx = 0;
  auto rightIdx = static_cast<int>(m_samples.size() - 1);
  double leftX = m_samples[leftIdx].x;
  while (leftIdx + 1 < rightIdx) {
    const int midIdx = (leftIdx + rightIdx) >> 1;
    const double midX = m_samples[midIdx].x;
    if ((x - midX) * (x - leftX) <= 0) {
      // Note: <= 0 vs < 0 is actually important for this branch.
      // 0 would indicate either left or mid point is our exact answer.
      rightIdx = midIdx;
    } else {
      leftIdx = midIdx;
      leftX = midX;
    }
  }

  hint.update(leftIdx);

  return interpolateXInSegment(x, leftIdx);
}  // ArcLengthMapper::xToArcLen

bool ArcLengthMapper::checkSegmentForArcLen(double arcLen, int segment) const {
  assert(m_samples.size() > 1);  // Enforced by the caller.
  if ((segment < 0) || (segment >= int(m_samples.size()) - 1)) {
    return false;
  }

  const double leftArcLen = m_samples[segment].arcLen;
  const double rightArcLen = m_samples[segment + 1].arcLen;

  return (arcLen - leftArcLen) * (arcLen - rightArcLen) <= 0;
}

bool ArcLengthMapper::checkSegmentForX(double x, int segment) const {
  assert(m_samples.size() > 1);  // Enforced by the caller.
  if ((segment < 0) || (segment >= int(m_samples.size()) - 1)) {
    return false;
  }

  const double leftX = m_samples[segment].x;
  const double rightX = m_samples[segment + 1].x;

  return (x - leftX) * (x - rightX) <= 0;
}

double ArcLengthMapper::interpolateArcLenInSegment(double arcLen, int segment) const {
  // a - a0   a1 - a0
  // ------ = -------
  // x - x0   x1 - x0
  //
  // x = x0 + (a - a0) * (x1 - x0) / (a1 - a0)

  const double x0 = m_samples[segment].x;
  const double a0 = m_samples[segment].arcLen;
  const double x1 = m_samples[segment + 1].x;
  const double a1 = m_samples[segment + 1].arcLen;
  const double x = x0 + (arcLen - a0) * (x1 - x0) / (a1 - a0);

  return x;
}

double ArcLengthMapper::interpolateXInSegment(double x, int segment) const {
  // a - a0   a1 - a0
  // ------ = -------
  // x - x0   x1 - x0
  //
  // a = a0 + (a1 - a0) * (x - x0) / (x1 - x0)

  const double x0 = m_samples[segment].x;
  const double a0 = m_samples[segment].arcLen;
  const double x1 = m_samples[segment + 1].x;
  const double a1 = m_samples[segment + 1].arcLen;
  const double a = a0 + (a1 - a0) * (x - x0) / (x1 - x0);

  return a;
}
