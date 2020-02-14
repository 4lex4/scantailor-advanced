// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_PARAMS_H_
#define SCANTAILOR_OUTPUT_PARAMS_H_

#include <dewarping/DistortionModel.h>

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


inline const Dpi& Params::outputDpi() const {
  return m_dpi;
}

inline void Params::setOutputDpi(const Dpi& dpi) {
  m_dpi = dpi;
}

inline const ColorParams& Params::colorParams() const {
  return m_colorParams;
}

inline const PictureShapeOptions& Params::pictureShapeOptions() const {
  return m_pictureShapeOptions;
}

inline void Params::setPictureShapeOptions(const PictureShapeOptions& opt) {
  m_pictureShapeOptions = opt;
}

inline void Params::setColorParams(const ColorParams& params) {
  m_colorParams = params;
}

inline const SplittingOptions& Params::splittingOptions() const {
  return m_splittingOptions;
}

inline void Params::setSplittingOptions(const SplittingOptions& opt) {
  m_splittingOptions = opt;
}

inline const DewarpingOptions& Params::dewarpingOptions() const {
  return m_dewarpingOptions;
}

inline void Params::setDewarpingOptions(const DewarpingOptions& opt) {
  m_dewarpingOptions = opt;
}

inline const dewarping::DistortionModel& Params::distortionModel() const {
  return m_distortionModel;
}

inline void Params::setDistortionModel(const dewarping::DistortionModel& model) {
  m_distortionModel = model;
}

inline const DepthPerception& Params::depthPerception() const {
  return m_depthPerception;
}

inline void Params::setDepthPerception(DepthPerception depthPerception) {
  m_depthPerception = depthPerception;
}

inline double Params::despeckleLevel() const {
  return m_despeckleLevel;
}

inline void Params::setDespeckleLevel(double level) {
  m_despeckleLevel = level;
}

inline bool Params::isBlackOnWhite() const {
  return m_blackOnWhite;
}

inline void Params::setBlackOnWhite(bool isBlackOnWhite) {
  Params::m_blackOnWhite = isBlackOnWhite;
}
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_PARAMS_H_
