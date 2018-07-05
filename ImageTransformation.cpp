/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ImageTransformation.h"

ImageTransformation::ImageTransformation(const QRectF& orig_image_rect, const Dpi& orig_dpi)
    : m_postRotation(0.0), m_origRect(orig_image_rect), m_resultingRect(orig_image_rect), m_origDpi(orig_dpi) {
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

  const QSizeF new_pre_scaled_image_size(m_origRect.width() * xscale, m_origRect.height() * yscale);

  // Undo's for the specified steps.
  const QTransform undo21(m_preRotateXform.inverted() * m_preScaleXform.inverted());
  const QTransform undo4321(m_postRotateXform.inverted() * m_preCropXform.inverted() * undo21);

  // Update transform #1: pre-scale.
  m_preScaleXform.reset();
  m_preScaleXform.scale(xscale, yscale);

  // Update transform #2: pre-rotate.
  m_preRotateXform = m_preRotation.transform(new_pre_scaled_image_size);

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
  const int min_dpi = std::min(m_origDpi.horizontal(), m_origDpi.vertical());
  preScaleToDpi(Dpi(min_dpi, min_dpi));
}

void ImageTransformation::setPreRotation(const OrthogonalRotation rotation) {
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
    const QPolygonF pre_rotate_poly(m_preCropXform.map(m_preCropArea));
    const QRectF pre_rotate_rect(pre_rotate_poly.boundingRect());
    const QPolygonF post_rotate_poly(xform.map(pre_rotate_poly));
    const QRectF post_rotate_rect(post_rotate_poly.boundingRect());

    xform *= QTransform().translate(pre_rotate_rect.left() - post_rotate_rect.left(),
                                    pre_rotate_rect.top() - post_rotate_rect.top());
  }

  return xform;
}

QTransform ImageTransformation::calcPostScaleXform(const Dpi& target_dpi) {
  if (target_dpi.isNull()) {
    return QTransform();
  }

  // We are going to measure the effective DPI after the previous transforms.
  // Normally m_preScaledDpi would be symmetric, so we could just
  // use that, but just in case ...

  const QTransform to_orig(m_postScaleXform * m_transform.inverted());
  // IMPORTANT: in the above line we assume post-scale is the last transform.

  const QLineF hor_unit(QPointF(0, 0), QPointF(1, 0));
  const QLineF vert_unit(QPointF(0, 0), QPointF(0, 1));
  const QLineF orig_hor_unit(to_orig.map(hor_unit));
  const QLineF orig_vert_unit(to_orig.map(vert_unit));

  const double xscale = target_dpi.horizontal() * orig_hor_unit.length() / m_origDpi.horizontal();
  const double yscale = target_dpi.vertical() * orig_vert_unit.length() / m_origDpi.vertical();
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
  const QTransform pre_scale_then_pre_rotate(m_preScaleXform * m_preRotateXform);                // 12
  const QTransform pre_crop_then_post_rotate(m_preCropXform * m_postRotateXform);                // 34
  const QTransform post_crop_then_post_scale(m_postCropXform * m_postScaleXform);                // 56
  const QTransform pre_crop_and_further(pre_crop_then_post_rotate * post_crop_then_post_scale);  // 3456
  m_transform = pre_scale_then_pre_rotate * pre_crop_and_further;
  m_invTransform = m_transform.inverted();
  if (m_preCropArea.empty()) {
    m_preCropArea = pre_scale_then_pre_rotate.map(m_origRect);
  }
  if (m_postCropArea.empty()) {
    m_postCropArea = pre_crop_then_post_rotate.map(m_preCropArea);
  }
  m_resultingPreCropArea = pre_crop_and_further.map(m_preCropArea);
  m_resultingPostCropArea = post_crop_then_post_scale.map(m_postCropArea);
  m_resultingRect = m_resultingPostCropArea.boundingRect();
}
