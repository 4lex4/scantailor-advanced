/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BubbleAnimation.h"
#include <QPaintDevice>
#include <QPainter>
#include <cassert>
#include <cmath>
#include "imageproc/ColorInterpolation.h"
#include "imageproc/Constants.h"

using namespace imageproc;

BubbleAnimation::BubbleAnimation(const int num_bubbles) : m_numBubbles(num_bubbles), m_curFrame(0) {
  assert(m_numBubbles > 0);
}

bool BubbleAnimation::nextFrame(const QColor& head_color, const QColor& tail_color, QPaintDevice* pd, QRectF rect) {
  if (rect.isNull()) {
    rect = QRectF(0.0, 0.0, pd->width(), pd->height());
  }

  QPainter painter(pd);

  return nextFrame(head_color, tail_color, &painter, rect);
}

bool BubbleAnimation::nextFrame(const QColor& head_color,
                                const QColor& tail_color,
                                QPainter* painter,
                                const QRectF rect) {
  const QPointF center(rect.center());
  const double radius = std::min(center.x() - rect.x(), center.y() - rect.y());

  const double PI = imageproc::constants::PI;
  const double arc_fraction_as_radius = 0.25;
  // We have the following system of equations:
  // bubble_radius = arc_between_bubbles * arc_fraction_as_radius;
  // arc_between_bubbles = 2.0 * PI * reduced_radius / m_numBubbles;
  // reduced_radius = radius - bubble_radius.
  // Solving this system of equations, we get:
  const double reduced_radius = radius / (1.0 + 2.0 * PI * arc_fraction_as_radius / m_numBubbles);
  const double bubble_radius = radius - reduced_radius;

  const double tail_length = 0.5 * m_numBubbles;

  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(Qt::NoPen);

  for (int i = 0; i < m_numBubbles; ++i) {
    const double angle = -0.5 * PI + 2.0 * PI * (m_curFrame - i) / m_numBubbles;
    const double s = std::sin(angle);
    const double c = std::cos(angle);
    const QPointF vec(c * reduced_radius, s * reduced_radius);
    QRectF r(0.0, 0.0, 2.0 * bubble_radius, 2.0 * bubble_radius);
    r.moveCenter(center + vec);
    const double color_dist = std::min(1.0, i / tail_length);
    painter->setBrush(colorInterpolation(head_color, tail_color, color_dist));
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
