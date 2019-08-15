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

#include "DewarpingPointMapper.h"
#include <QTransform>
#include "DistortionModel.h"

namespace dewarping {
DewarpingPointMapper::DewarpingPointMapper(const DistortionModel& distortion_model,
                                           double depth_perception,
                                           const QTransform& distortion_model_to_output,
                                           const QRect& output_content_rect,
                                           const QTransform& postTransform)
    : m_dewarper(CylindricalSurfaceDewarper(distortion_model.topCurve().polyline(),
                                            distortion_model.bottomCurve().polyline(),
                                            depth_perception)),
      m_postTransform(postTransform) {
  // Model domain is a rectangle in output image coordinates that
  // will be mapped to our curved quadrilateral.
  const QRect model_domain(
      distortion_model.modelDomain(m_dewarper, distortion_model_to_output, output_content_rect).toRect());

  // Note: QRect::right() - QRect::left() will give you size() - 1 not size()!
  // That's intended.

  m_modelDomainLeft = model_domain.left();
  m_modelXScaleFromNormalized = model_domain.right() - model_domain.left();
  m_modelXScaleToNormalized = 1.0 / m_modelXScaleFromNormalized;

  m_modelDomainTop = model_domain.top();
  m_modelYScaleFromNormalized = model_domain.bottom() - model_domain.top();
  m_modelYScaleToNormalized = 1.0 / m_modelYScaleFromNormalized;
}

QPointF DewarpingPointMapper::mapToDewarpedSpace(const QPointF& warped_pt) const {
  const QPointF crv_pt(m_dewarper.mapToDewarpedSpace(warped_pt));
  const double dewarped_x = crv_pt.x() * m_modelXScaleFromNormalized + m_modelDomainLeft;
  const double dewarped_y = crv_pt.y() * m_modelYScaleFromNormalized + m_modelDomainTop;

  return m_postTransform.map(QPointF(dewarped_x, dewarped_y));
}

QPointF DewarpingPointMapper::mapToWarpedSpace(const QPointF& dewarped_pt) const {
  QPointF dewarped_pt_m = m_postTransform.inverted().map(dewarped_pt);

  const double crv_x = (dewarped_pt_m.x() - m_modelDomainLeft) * m_modelXScaleToNormalized;
  const double crv_y = (dewarped_pt_m.y() - m_modelDomainTop) * m_modelYScaleToNormalized;

  return m_dewarper.mapToWarpedSpace(QPointF(crv_x, crv_y));
}
}  // namespace dewarping