// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_PARAMS_H_
#define SCANTAILOR_OUTPUT_PARAMS_H_

#include <DistortionModel.h>
#include "ColorParams.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "DewarpingOptions.h"
#include "Dpi.h"
#include "PictureShapeOptions.h"

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

  void setDepthPerception(DepthPerception depthPerception);

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
#endif  // ifndef SCANTAILOR_OUTPUT_PARAMS_H_
