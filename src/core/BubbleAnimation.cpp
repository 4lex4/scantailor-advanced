// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BubbleAnimation.h"

#include <ColorInterpolation.h>
#include <Constants.h>

#include <QPaintDevice>
#include <QPainter>
#include <cassert>
#include <cmath>

using namespace imageproc;

BubbleAnimation::BubbleAnimation(const int numBubbles) : m_numBubbles(numBubbles), m_curFrame(0) {
  assert(m_numBubbles > 0);
}

bool BubbleAnimation::nextFrame(const QColor& headColor, const QColor& tailColor, QPaintDevice* pd, QRectF rect) {
  if (rect.isNull()) {
    rect = QRectF(0.0, 0.0, pd->width(), pd->height());
  }

  QPainter painter(pd);
  return nextFrame(headColor, tailColor, &painter, rect);
}

bool BubbleAnimation::nextFrame(const QColor& headColor,
                                const QColor& tailColor,
                                QPainter* painter,
                                const QRectF rect) {
  const QPointF center(rect.center());
  const double radius = std::min(center.x() - rect.x(), center.y() - rect.y());

  const double PI = constants::PI;
  const double arcFractionAsRadius = 0.25;
  // We have the following system of equations:
  // bubbleRadius = arc_between_bubbles * arcFractionAsRadius;
  // arc_between_bubbles = 2.0 * PI * reducedRadius / m_numBubbles;
  // reducedRadius = radius - bubbleRadius.
  // Solving this system of equations, we get:
  const double reducedRadius = radius / (1.0 + 2.0 * PI * arcFractionAsRadius / m_numBubbles);
  const double bubbleRadius = radius - reducedRadius;

  const double tailLength = 0.5 * m_numBubbles;

  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(Qt::NoPen);

  for (int i = 0; i < m_numBubbles; ++i) {
    const double angle = -0.5 * PI + 2.0 * PI * (m_curFrame - i) / m_numBubbles;
    const double s = std::sin(angle);
    const double c = std::cos(angle);
    const QPointF vec(c * reducedRadius, s * reducedRadius);
    QRectF r(0.0, 0.0, 2.0 * bubbleRadius, 2.0 * bubbleRadius);
    r.moveCenter(center + vec);
    const double colorDist = std::min(1.0, i / tailLength);
    painter->setBrush(colorInterpolation(headColor, tailColor, colorDist));
    painter->drawEllipse(r);
  }

  if (m_curFrame + 1 < m_numBubbles) {
    ++m_curFrame;
    return true;
  } else {
    m_curFrame = 0;
    return false;
  }
}  // BubbleAnimation::nextFrame
