// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DEWARPING_DEWARPINGPOINTMAPPER_H_
#define SCANTAILOR_DEWARPING_DEWARPINGPOINTMAPPER_H_

#include <QtGui/QTransform>
#include "CylindricalSurfaceDewarper.h"

class QRect;

namespace dewarping {
class DistortionModel;

class DewarpingPointMapper {
 public:
  DewarpingPointMapper(const dewarping::DistortionModel& distortionModel,
                       double depthPerception,
                       const QTransform& distortionModelToOutput,
                       const QRect& outputContentRect,
                       const QTransform& postTransform = QTransform());

  /**
   * Similar to CylindricalSurfaceDewarper::mapToDewarpedSpace(),
   * except it maps to dewarped image coordinates rather than
   * to normalized dewarped coordinates.
   */
  QPointF mapToDewarpedSpace(const QPointF& warpedPt) const;

  /**
   * Similar to CylindricalSurfaceDewarper::mapToWarpedSpace(),
   * except it maps from dewarped image coordinates rather than
   * from normalized dewarped coordinates.
   */
  QPointF mapToWarpedSpace(const QPointF& dewarpedPt) const;

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
#endif  // ifndef SCANTAILOR_DEWARPING_DEWARPINGPOINTMAPPER_H_
