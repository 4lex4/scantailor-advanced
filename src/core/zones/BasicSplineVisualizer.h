// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_BASICSPLINEVISUALIZER_H_
#define SCANTAILOR_ZONES_BASICSPLINEVISUALIZER_H_

#include <QColor>
#include <QPen>
#include "EditableSpline.h"

class EditableZoneSet;
class QPainter;
class QTransform;

class BasicSplineVisualizer {
 public:
  BasicSplineVisualizer();

  QRgb solidColor() const { return m_solidColor; }

  QRgb highlightBrightColor() const { return m_highlightBrightColor; }

  QRgb highlightDarkColor() const { return m_highlightDarkColor; }

  void drawVertex(QPainter& painter, const QPointF& pt, const QColor& color);

  void drawSplines(QPainter& painter, const QTransform& toScreen, const EditableZoneSet& zones);

  virtual void drawSpline(QPainter& painter, const QTransform& toScreen, const EditableSpline::Ptr& spline);

  virtual void prepareForSpline(QPainter& painter, const EditableSpline::Ptr& spline);

 protected:
  QRgb m_solidColor;
  QRgb m_highlightBrightColor;
  QRgb m_highlightDarkColor;
  QPen m_pen;
};


#endif  // ifndef SCANTAILOR_ZONES_BASICSPLINEVISUALIZER_H_
