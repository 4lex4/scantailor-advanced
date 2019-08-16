// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FrenetFrame.h"
#include <cmath>

namespace spfit {
FrenetFrame::FrenetFrame(const Vec2d& origin, const Vec2d& tangent_vector, YAxisDirection ydir) : m_origin(origin) {
  const double sqlen = tangent_vector.squaredNorm();
  if (sqlen > 1e-6) {
    m_unitTangent = tangent_vector / std::sqrt(sqlen);
    if (ydir == Y_POINTS_UP) {
      m_unitNormal[0] = -m_unitTangent[1];
      m_unitNormal[1] = m_unitTangent[0];
    } else {
      m_unitNormal[0] = m_unitTangent[1];
      m_unitNormal[1] = -m_unitTangent[0];
    }
  }
}
}  // namespace spfit
