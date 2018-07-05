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

#ifndef OUTPUT_PARAMS_H_
#define OUTPUT_PARAMS_H_

#include "ColorParams.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "DewarpingOptions.h"
#include "Dpi.h"
#include "PictureShapeOptions.h"
#include "dewarping/DistortionModel.h"

class QDomDocument;
class QDomElement;

namespace output {
class Params {
 public:
  Params();

  Params(const Dpi& dpi,
         const ColorParams& colorParams,
         const SplittingOptions& splittingOptions,
         const PictureShapeOptions& pictureShapeOptions,
         const dewarping::DistortionModel& distortionModel,
         const DepthPerception& depthPerception,
         const DewarpingOptions& dewarpingOptions,
         double despeckleLevel);

  explicit Params(const QDomElement& el);

  const Dpi& outputDpi() const;

  void setOutputDpi(const Dpi& dpi);

  const ColorParams& colorParams() const;

  const PictureShapeOptions& pictureShapeOptions() const;

  void setPictureShapeOptions(const PictureShapeOptions& opt);

  void setColorParams(const ColorParams& params);

  const SplittingOptions& splittingOptions() const;

  void setSplittingOptions(const SplittingOptions& opt);

  const DewarpingOptions& dewarpingOptions() const;

  void setDewarpingOptions(const DewarpingOptions& opt);

  const dewarping::DistortionModel& distortionModel() const;

  void setDistortionModel(const dewarping::DistortionModel& model);

  const DepthPerception& depthPerception() const;

  void setDepthPerception(DepthPerception depth_perception);

  double despeckleLevel() const;

  void setDespeckleLevel(double level);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool isBlackOnWhite() const;

  void setBlackOnWhite(bool isBlackOnWhite);

 private:
  Dpi m_dpi;
  ColorParams m_colorParams;
  SplittingOptions m_splittingOptions;
  PictureShapeOptions m_pictureShapeOptions;
  dewarping::DistortionModel m_distortionModel;
  DepthPerception m_depthPerception;
  DewarpingOptions m_dewarpingOptions;
  double m_despeckleLevel;
  bool m_blackOnWhite;
};
}  // namespace output
#endif  // ifndef OUTPUT_PARAMS_H_
