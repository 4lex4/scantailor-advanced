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

#ifndef BASIC_SPLINE_VISUALIZER_H_
#define BASIC_SPLINE_VISUALIZER_H_

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

  void drawSplines(QPainter& painter, const QTransform& to_screen, const EditableZoneSet& zones);

  virtual void drawSpline(QPainter& painter, const QTransform& to_screen, const EditableSpline::Ptr& spline);

  virtual void prepareForSpline(QPainter& painter, const EditableSpline::Ptr& spline);

 protected:
  QRgb m_solidColor;
  QRgb m_highlightBrightColor;
  QRgb m_highlightDarkColor;
  QPen m_pen;
};


#endif  // ifndef BASIC_SPLINE_VISUALIZER_H_
