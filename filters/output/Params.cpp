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

#include "Params.h"
#include "../../Utils.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

namespace output {
Params::Params() : m_dpi(600, 600), m_despeckleLevel(1.0) {}

Params::Params(const Dpi& dpi,
               const ColorParams& colorParams,
               const SplittingOptions& splittingOptions,
               const PictureShapeOptions& pictureShapeOptions,
               const dewarping::DistortionModel& distortionModel,
               const DepthPerception& depthPerception,
               const DewarpingOptions& dewarpingOptions,
               const double despeckleLevel)
    : m_dpi(dpi),
      m_colorParams(colorParams),
      m_splittingOptions(splittingOptions),
      m_pictureShapeOptions(pictureShapeOptions),
      m_distortionModel(distortionModel),
      m_depthPerception(depthPerception),
      m_dewarpingOptions(dewarpingOptions),
      m_despeckleLevel(despeckleLevel),
      m_blackOnWhite(true) {}

Params::Params(const QDomElement& el)
    : m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
      m_colorParams(el.namedItem("color-params").toElement()),
      m_distortionModel(el.namedItem("distortion-model").toElement()),
      m_depthPerception(el.attribute("depthPerception")),
      m_dewarpingOptions(el.namedItem("dewarping-options").toElement()),
      m_despeckleLevel(el.attribute("despeckleLevel").toDouble()),
      m_pictureShapeOptions(el.namedItem("picture-shape-options").toElement()),
      m_splittingOptions(el.namedItem("splitting").toElement()),
      m_blackOnWhite(el.attribute("blackOnWhite") == "1") {}

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(m_distortionModel.toXml(doc, "distortion-model"));
  el.appendChild(m_pictureShapeOptions.toXml(doc, "picture-shape-options"));
  el.setAttribute("depthPerception", m_depthPerception.toString());
  el.appendChild(m_dewarpingOptions.toXml(doc, "dewarping-options"));
  el.setAttribute("despeckleLevel", Utils::doubleToString(m_despeckleLevel));
  el.appendChild(marshaller.dpi(m_dpi, "dpi"));
  el.appendChild(m_colorParams.toXml(doc, "color-params"));
  el.appendChild(m_splittingOptions.toXml(doc, "splitting"));
  el.setAttribute("blackOnWhite", m_blackOnWhite ? "1" : "0");

  return el;
}

const Dpi& Params::outputDpi() const {
  return m_dpi;
}

void Params::setOutputDpi(const Dpi& dpi) {
  m_dpi = dpi;
}

const ColorParams& Params::colorParams() const {
  return m_colorParams;
}

const PictureShapeOptions& Params::pictureShapeOptions() const {
  return m_pictureShapeOptions;
}

void Params::setPictureShapeOptions(const PictureShapeOptions& opt) {
  m_pictureShapeOptions = opt;
}

void Params::setColorParams(const ColorParams& params) {
  m_colorParams = params;
}

const SplittingOptions& Params::splittingOptions() const {
  return m_splittingOptions;
}

void Params::setSplittingOptions(const SplittingOptions& opt) {
  m_splittingOptions = opt;
}

const DewarpingOptions& Params::dewarpingOptions() const {
  return m_dewarpingOptions;
}

void Params::setDewarpingOptions(const DewarpingOptions& opt) {
  m_dewarpingOptions = opt;
}

const dewarping::DistortionModel& Params::distortionModel() const {
  return m_distortionModel;
}

void Params::setDistortionModel(const dewarping::DistortionModel& model) {
  m_distortionModel = model;
}

const DepthPerception& Params::depthPerception() const {
  return m_depthPerception;
}

void Params::setDepthPerception(DepthPerception depth_perception) {
  m_depthPerception = depth_perception;
}

double Params::despeckleLevel() const {
  return m_despeckleLevel;
}

void Params::setDespeckleLevel(double level) {
  m_despeckleLevel = level;
}

bool Params::isBlackOnWhite() const {
  return m_blackOnWhite;
}

void Params::setBlackOnWhite(bool isBlackOnWhite) {
  Params::m_blackOnWhite = isBlackOnWhite;
}
}  // namespace output