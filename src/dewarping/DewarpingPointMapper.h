// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DEWARPING_DEWARPING_POINT_MAPPER_H_
#define DEWARPING_DEWARPING_POINT_MAPPER_H_

#include <QtGui/QTransform>
#include "CylindricalSurfaceDewarper.h"

class QRect;

namespace dewarping {
class DistortionModel;

class DewarpingPointMapper {
 public:
  DewarpingPointMapper(const dewarping::DistortionModel& distortion_model,
                       double depth_perception,
                       const QTransform& distortion_model_to_output,
                       const QRect& output_content_rect,
                       const QTransform& postTransform = QTransform());

  /**
   * Similar to CylindricalSurfaceDewarper::mapToDewarpedSpace(),
   * except it maps to dewarped image coordinates rather than
   * to normalized dewarped coordinates.
   */
  QPointF mapToDewarpedSpace(const QPointF& warped_pt) const;

  /**
   * Similar to CylindricalSurfaceDewarper::mapToWarpedSpace(),
   * except it maps from dewarped image coordinates rather than
   * from normalized dewarped coordinates.
   */
  QPointF mapToWarpedSpace(const QPointF& dewarped_pt) const;

 private:
  CylindricalSurfaceDewarper m_dewarper;
  double m_modelDomainLeft;
  double m_modelDomainTop;
  double m_modelXScaleFromNormalized;
  double m_modelYScaleFromNormalized;
  double m_modelXScaleToNormalized;
  double m_modelYScaleToNormalized;
  QTransform m_postTransform;
};
}  // namespace dewarping
#endif  // ifndef DEWARPING_DEWARPING_POINT_MAPPER_H_
