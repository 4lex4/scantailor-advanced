// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageTransformation.h"

ImageTransformation::ImageTransformation(const QRectF& origImageRect, const Dpi& origDpi)
    : m_postRotation(0.0), m_origRect(origImageRect), m_resultingRect(origImageRect), m_origDpi(origDpi) {
  preScaleToEqualizeDpi();
}

ImageTransformation::~ImageTransformation() = default;

void ImageTransformation::preScaleToDpi(const Dpi& dpi) {
  if (m_origDpi.isNull() || dpi.isNull()) {
    return;
  }

  m_preScaledDpi = dpi;

  const double xscale = (double) dpi.horizontal() / m_origDpi.horizontal();
  const double yscale = (double) dpi.vertical() / m_origDpi.vertical();

  const QSizeF newPreScaledImageSize(m_origRect.width() * xscale, m_origRect.height() * yscale);

  // Undo's for the specified steps.
  const QTransform undo21(m_preRotateXform.inverted() * m_preScaleXform.inverted());
  const QTransform undo4321(m_postRotateXform.inverted() * m_preCropXform.inverted() * undo21);

  // Update transform #1: pre-scale.
  m_preScaleXform.reset();
  m_preScaleXform.scale(xscale, yscale);

  // Update transform #2: pre-rotate.
  m_preRotateXform = m_preRotation.transform(newPreScaledImageSize);

  // Update transform #3: pre-crop.
  const QTransform redo12(m_preScaleXform * m_preRotateXform);
  m_preCropArea = (undo21 * redo12).map(m_preCropArea);
  m_preCropXform = calcCropXform(m_preCropArea);

  // Update transform #4: post-rotate.
  m_postRotateXform = calcPostRotateXform(m_postRotation);

  // Update transform #5: post-crop.
  const QTransform redo1234(redo12 * m_preCropXform * m_postRotateXform);
  m_postCropArea = (undo4321 * redo1234).map(m_postCropArea);
  m_postCropXform = calcCropXform(m_postCropArea);
  // Update transform #6: post-scale.
  m_postScaleXform = calcPostScaleXform(m_postScaledDpi);

  update();
}  // ImageTransformation::preScaleToDpi

void ImageTransformation::preScaleToEqualizeDpi() {
  const int minDpi = std::min(m_origDpi.horizontal(), m_origDpi.vertical());
  preScaleToDpi(Dpi(minDpi, minDpi));
}

void ImageTransformation::setPreRotation(const OrthogonalRotation& rotation) {
  m_preRotation = rotation;
  m_preRotateXform = m_preRotation.transform(m_origRect.size());
  resetPreCropArea();
  resetPostRotation();
  resetPostCrop();
  resetPostScale();
  update();
}

void ImageTransformation::setPreCropArea(const QPolygonF& area) {
  m_preCropArea = area;
  m_preCropXform = calcCropXform(area);
  resetPostRotation();
  resetPostCrop();
  resetPostScale();
  update();
}

void ImageTransformation::setPostRotation(const double degrees) {
  m_postRotateXform = calcPostRotateXform(degrees);
  m_postRotation = degrees;
  resetPostCrop();
  resetPostScale();
  update();
}

void ImageTransformation::setPostCropArea(const QPolygonF& area) {
  m_postCropArea = area;
  m_postCropXform = calcCropXform(area);
  resetPostScale();
  update();
}

void ImageTransformation::postScaleToDpi(const Dpi& dpi) {
  m_postScaledDpi = dpi;
  m_postScaleXform = calcPostScaleXform(dpi);
  update();
}

QTransform ImageTransformation::calcCropXform(const QPolygonF& area) {
  const QRectF bounds(area.boundingRect());
  QTransform xform;
  xform.translate(-bounds.x(), -bounds.y());

  return xform;
}

QTransform ImageTransformation::calcPostRotateXform(const double degrees) {
  QTransform xform;
  if (degrees != 0.0) {
    const QPointF origin(m_preCropArea.boundingRect().center());
    xform.translate(-origin.x(), -origin.y());
    xform *= QTransform().rotate(degrees);
    xform *= QTransform().translate(origin.x(), origin.y());

    // Calculate size changes.
    const QPolygonF preRotatePoly(m_preCropXform.map(m_preCropArea));
    const QRectF preRotateRect(preRotatePoly.boundingRect());
    const QPolygonF postRotatePoly(xform.map(preRotatePoly));
    const QRectF postRotateRect(postRotatePoly.boundingRect());

    xform *= QTransform().translate(preRotateRect.left() - postRotateRect.left(),
                                    preRotateRect.top() - postRotateRect.top());
  }

  return xform;
}

QTransform ImageTransformation::calcPostScaleXform(const Dpi& targetDpi) {
  if (targetDpi.isNull()) {
    return QTransform();
  }

  // We are going to measure the effective DPI after the previous transforms.
  // Normally m_preScaledDpi would be symmetric, so we could just
  // use that, but just in case ...

  const QTransform toOrig(m_postScaleXform * m_transform.inverted());
  // IMPORTANT: in the above line we assume post-scale is the last transform.

  const QLineF horUnit(QPointF(0, 0), QPointF(1, 0));
  const QLineF vertUnit(QPointF(0, 0), QPointF(0, 1));
  const QLineF origHorUnit(toOrig.map(horUnit));
  const QLineF origVertUnit(toOrig.map(vertUnit));

  const double xscale = targetDpi.horizontal() * origHorUnit.length() / m_origDpi.horizontal();
  const double yscale = targetDpi.vertical() * origVertUnit.length() / m_origDpi.vertical();
  QTransform xform;
  xform.scale(xscale, yscale);

  return xform;
}

void ImageTransformation::resetPreCropArea() {
  m_preCropArea.clear();
  m_preCropXform.reset();
}

void ImageTransformation::resetPostRotation() {
  m_postRotation = 0.0;
  m_postRotateXform.reset();
}

void ImageTransformation::resetPostCrop() {
  m_postCropArea.clear();
  m_postCropXform.reset();
}

void ImageTransformation::resetPostScale() {
  m_postScaledDpi = Dpi();
  m_postScaleXform.reset();
}

void ImageTransformation::update() {
  const QTransform preScaleThenPreRotate(m_preScaleXform * m_preRotateXform);         // 12
  const QTransform preCropThenPostRotate(m_preCropXform * m_postRotateXform);         // 34
  const QTransform postCropThenPostScale(m_postCropXform * m_postScaleXform);         // 56
  const QTransform preCropAndFurther(preCropThenPostRotate * postCropThenPostScale);  // 3456
  m_transform = preScaleThenPreRotate * preCropAndFurther;
  m_invTransform = m_transform.inverted();
  if (m_preCropArea.empty()) {
    m_preCropArea = preScaleThenPreRotate.map(m_origRect);
  }
  if (m_postCropArea.empty()) {
    m_postCropArea = preCropThenPostRotate.map(m_preCropArea);
  }
  m_resultingPreCropArea = preCropAndFurther.map(m_preCropArea);
  m_resultingPostCropArea = postCropThenPostScale.map(m_postCropArea);
  m_resultingRect = m_resultingPostCropArea.boundingRect();
}
