// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BasicSplineVisualizer.h"
#include <QPainter>
#include "EditableZoneSet.h"

BasicSplineVisualizer::BasicSplineVisualizer()
    : m_solidColor(0xcc1420), m_highlightBrightColor(0xfffe00), m_highlightDarkColor(0xffa90e), m_pen(m_solidColor) {
  m_pen.setCosmetic(true);
  m_pen.setWidthF(1.5);
}

void BasicSplineVisualizer::drawSplines(QPainter& painter, const QTransform& toScreen, const EditableZoneSet& zones) {
  for (const EditableZoneSet::Zone& zone : zones) {
    drawSpline(painter, toScreen, zone.spline());
  }
}

void BasicSplineVisualizer::drawSpline(QPainter& painter,
                                       const QTransform& toScreen,
                                       const EditableSpline::Ptr& spline) {
  prepareForSpline(painter, spline);
  painter.drawPolygon(toScreen.map(spline->toPolygon()), Qt::WindingFill);
}

void BasicSplineVisualizer::drawVertex(QPainter& painter, const QPointF& pt, const QColor& color) {
  painter.setPen(Qt::NoPen);
  painter.setBrush(color);

  QRectF rect(0, 0, 4, 4);
  rect.moveCenter(pt);
  painter.drawEllipse(rect);
}

void BasicSplineVisualizer::prepareForSpline(QPainter& painter, const EditableSpline::Ptr&) {
  painter.setPen(m_pen);
  painter.setBrush(Qt::NoBrush);
}
