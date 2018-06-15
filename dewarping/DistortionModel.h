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

#ifndef DEWARPING_DISTORTION_MODEL_H_
#define DEWARPING_DISTORTION_MODEL_H_

#include "Curve.h"

class QDomDocument;
class QDomElement;
class QString;
class QRectF;
class QTransform;

namespace dewarping {
class CylindricalSurfaceDewarper;

class DistortionModel {
 public:
  /**
   * \brief Constructs a null distortion model.
   */
  DistortionModel();

  explicit DistortionModel(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  /**
   * Returns true if the model is not null and in addition meets certain
   * criteria, like curve endpoints forming a convex quadrilateral.
   */
  bool isValid() const;

  void setTopCurve(const Curve& curve) { m_topCurve = curve; }

  void setBottomCurve(const Curve& curve) { m_bottomCurve = curve; }

  const Curve& topCurve() const { return m_topCurve; }

  const Curve& bottomCurve() const { return m_bottomCurve; }

  bool matches(const DistortionModel& other) const;

  /**
   * Model domain is a rectangle in output image coordinates that
   * will be mapped to our curved quadrilateral.
   */
  QRectF modelDomain(const CylindricalSurfaceDewarper& dewarper,
                     const QTransform& to_output,
                     const QRectF& output_content_rect) const;

 private:
  /**
   * \return The bounding box of the shape formed by two curves
   *         and vertical segments connecting them.
   * \param transform Transforms from the original image coordinates
   *        where curve points are defined, to the desired coordinate
   *        system, for example to output image coordinates.
   */
  QRectF boundingBox(const QTransform& transform) const;

  Curve m_topCurve;
  Curve m_bottomCurve;
};
}  // namespace dewarping
#endif  // ifndef DEWARPING_DISTORTION_MODEL_H_
