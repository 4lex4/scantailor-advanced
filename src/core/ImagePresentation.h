// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEPRESENTATION_H_
#define SCANTAILOR_CORE_IMAGEPRESENTATION_H_

#include <QPolygonF>
#include <QRectF>
#include <QTransform>

/**
 * Image presentation consists of 3 components:
 * \li A transformation from pixel coordinates to some arbitrary virtual coordinates.
 *     Such transformation usually comes from an instance of ImageTransformation.
 * \li Image crop area, in virtual coordinates, that specifies which parts of the
 *     image should be visible.
 * \li Display area, in virtual coordinates, which is usually the bounding box
 *     of the image crop area, but can be larger, to reserve space for custom drawing
 *     or extra controls.
 */
class ImagePresentation {
  // Member-wise copying is OK.
 public:
  ImagePresentation(const QTransform& xform, const QPolygonF& cropArea)
      : m_xform(xform), m_cropArea(cropArea), m_displayArea(cropArea.boundingRect()) {}

  ImagePresentation(const QTransform& xform, const QPolygonF& cropArea, const QRectF& displayArea)
      : m_xform(xform), m_cropArea(cropArea), m_displayArea(displayArea) {}

  const QTransform& transform() const { return m_xform; }

  void setTransform(const QTransform& xform) { m_xform = xform; }

  const QPolygonF& cropArea() const { return m_cropArea; }

  void setCropArea(const QPolygonF& cropArea) { m_cropArea = cropArea; }

  const QRectF& displayArea() const { return m_displayArea; }

  void setDisplayArea(const QRectF& displayArea) { m_displayArea = displayArea; }

 private:
  QTransform m_xform;
  QPolygonF m_cropArea;
  QRectF m_displayArea;
};


#endif  // ifndef SCANTAILOR_CORE_IMAGEPRESENTATION_H_
