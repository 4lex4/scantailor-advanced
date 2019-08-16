// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SPFIT_FRENET_FRAME_H_
#define SPFIT_FRENET_FRAME_H_

#include "VecNT.h"

namespace spfit {
/**
 * Origin + unit tangent + unit normal vectors.
 */
class FrenetFrame {
  // Member-wise copying is OK.
 public:
  enum YAxisDirection { Y_POINTS_UP, Y_POINTS_DOWN };

  /**
   * \brief Builds a Frenet frame from an origin and a (non-unit) tangent vector.
   *
   * The direction of the normal vector is choosen according to \p ydir,
   * considering the tangent vector to be pointing to the right.  The normal direction
   * does matter, as we want the unit normal vector divided by signed curvature give
   * us the center of the curvature.  For that to be the case, normal vector's direction
   * relative to the unit vector's direction must be the same as the Y axis direction
   * relative to the X axis direction in the coordinate system from which we derive
   * the curvature.  For 2D computer graphics, the right direction is Y_POINTS_DOWN.
   */
  FrenetFrame(const Vec2d& origin, const Vec2d& tangent_vector, YAxisDirection ydir = Y_POINTS_DOWN);

  const Vec2d& origin() const { return m_origin; }

  const Vec2d& unitTangent() const { return m_unitTangent; }

  const Vec2d& unitNormal() const { return m_unitNormal; }

 private:
  Vec2d m_origin;
  Vec2d m_unitTangent;
  Vec2d m_unitNormal;
};
}  // namespace spfit
#endif  // ifndef SPFIT_FRENET_FRAME_H_
