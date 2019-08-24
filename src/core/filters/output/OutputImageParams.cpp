// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputImageParams.h"
#include <foundation/Utils.h>
#include <cmath>
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

using namespace foundation;

namespace output {
OutputImageParams::OutputImageParams(const QSize& outImageSize,
                                     const QRect& contentRect,
                                     ImageTransformation xform,
                                     const Dpi& dpi,
                                     const ColorParams& colorParams,
                                     const SplittingOptions& splittingOptions,
                                     const DewarpingOptions& dewarpingOptions,
                                     const dewarping::DistortionModel& distortionModel,
                                     const DepthPerception& depthPerception,
                                     const double despeckleLevel,
                                     const PictureShapeOptions& pictureShapeOptions,
                                     const OutputProcessingParams& outputProcessingParams,
                                     bool isBlackOnWhite)
    : m_size(outImageSize),
      m_contentRect(contentRect),
      m_cropArea(xform.resultingPreCropArea()),
      m_dpi(dpi),
      m_colorParams(colorParams),
      m_splittingOptions(splittingOptions),
      m_pictureShapeOptions(pictureShapeOptions),
      m_distortionModel(distortionModel),
      m_depthPerception(depthPerception),
      m_dewarpingOptions(dewarpingOptions),
      m_despeckleLevel(despeckleLevel),
      m_outputProcessingParams(outputProcessingParams),
      m_blackOnWhite(isBlackOnWhite) {
  // For historical reasons, we disregard post-cropping and post-scaling here.
  xform.setPostCropArea(QPolygonF());  // Resets post-scale as well.
  m_partialXform = xform.transform();
}

OutputImageParams::OutputImageParams(const QDomElement& el)
    : m_size(XmlUnmarshaller::size(el.namedItem("size").toElement())),
      m_contentRect(XmlUnmarshaller::rect(el.namedItem("content-rect").toElement())),
      m_cropArea(XmlUnmarshaller::polygonF(el.namedItem("crop-area").toElement())),
      m_partialXform(el.namedItem("partial-xform").toElement()),
      m_dpi(el.namedItem("dpi").toElement()),
      m_colorParams(el.namedItem("color-params").toElement()),
      m_splittingOptions(el.namedItem("splitting").toElement()),
      m_pictureShapeOptions(el.namedItem("picture-shape-options").toElement()),
      m_distortionModel(el.namedItem("distortion-model").toElement()),
      m_depthPerception(el.attribute("depthPerception")),
      m_dewarpingOptions(el.namedItem("dewarping-options").toElement()),
      m_despeckleLevel(el.attribute("despeckleLevel").toDouble()),
      m_outputProcessingParams(el.namedItem("processing-params").toElement()),
      m_blackOnWhite(el.attribute("blackOnWhite") == "1") {}

QDomElement OutputImageParams::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(marshaller.size(m_size, "size"));
  el.appendChild(marshaller.rect(m_contentRect, "content-rect"));
  el.appendChild(marshaller.polygonF(m_cropArea, "crop-area"));
  el.appendChild(m_partialXform.toXml(doc, "partial-xform"));
  el.appendChild(m_dpi.toXml(doc, "dpi"));
  el.appendChild(m_colorParams.toXml(doc, "color-params"));
  el.appendChild(m_splittingOptions.toXml(doc, "splitting"));
  el.appendChild(m_pictureShapeOptions.toXml(doc, "picture-shape-options"));
  el.appendChild(m_distortionModel.toXml(doc, "distortion-model"));
  el.setAttribute("depthPerception", m_depthPerception.toString());
  el.appendChild(m_dewarpingOptions.toXml(doc, "dewarping-options"));
  el.setAttribute("despeckleLevel", Utils::doubleToString(m_despeckleLevel));
  el.appendChild(m_outputProcessingParams.toXml(doc, "processing-params"));
  el.setAttribute("blackOnWhite", m_blackOnWhite ? "1" : "0");

  return el;
}

bool OutputImageParams::matches(const OutputImageParams& other) const {
  if (m_size != other.m_size) {
    return false;
  }

  if (m_contentRect != other.m_contentRect) {
    return false;
  }

  if (m_cropArea != other.m_cropArea) {
    return false;
  }

  if (!m_partialXform.matches(other.m_partialXform)) {
    return false;
  }

  if (m_dpi != other.m_dpi) {
    return false;
  }

  if (!colorParamsMatch(m_colorParams, m_despeckleLevel, m_splittingOptions, other.m_colorParams,
                        other.m_despeckleLevel, other.m_splittingOptions)) {
    return false;
  }

  if (m_pictureShapeOptions != other.m_pictureShapeOptions) {
    return false;
  }

  if (m_dewarpingOptions != other.m_dewarpingOptions) {
    return false;
  } else if (m_dewarpingOptions.dewarpingMode() != OFF) {
    if (!m_distortionModel.matches(other.m_distortionModel)) {
      return false;
    }
    if (m_depthPerception.value() != other.m_depthPerception.value()) {
      return false;
    }
  }

  if (m_outputProcessingParams != other.m_outputProcessingParams) {
    return false;
  }

  if (m_blackOnWhite != other.m_blackOnWhite) {
    return false;
  }

  return true;
}  // OutputImageParams::matches

bool OutputImageParams::colorParamsMatch(const ColorParams& cp1,
                                         const double dl1,
                                         const SplittingOptions& so1,
                                         const ColorParams& cp2,
                                         const double dl2,
                                         const SplittingOptions& so2) {
  if (cp1.colorMode() != cp2.colorMode()) {
    return false;
  }

  switch (cp1.colorMode()) {
    case MIXED:
      if (so1 != so2) {
        return false;
      }
      // fall through
    case BLACK_AND_WHITE:
      if (cp1.blackWhiteOptions() != cp2.blackWhiteOptions()) {
        return false;
      }
      if (dl1 != dl2) {
        return false;
      }
      // fall through
    case COLOR_GRAYSCALE:
      if (cp1.colorCommonOptions() != cp2.colorCommonOptions()) {
        return false;
      }
      break;
  }

  return true;
}

void OutputImageParams::setOutputProcessingParams(const OutputProcessingParams& outputProcessingParams) {
  OutputImageParams::m_outputProcessingParams = outputProcessingParams;
}

const PictureShapeOptions& OutputImageParams::getPictureShapeOptions() const {
  return m_pictureShapeOptions;
}

const QPolygonF& OutputImageParams::getCropArea() const {
  return m_cropArea;
}

const DewarpingOptions& OutputImageParams::dewarpingMode() const {
  return m_dewarpingOptions;
}

const dewarping::DistortionModel& OutputImageParams::distortionModel() const {
  return m_distortionModel;
}

void OutputImageParams::setDistortionModel(const dewarping::DistortionModel& model) {
  m_distortionModel = model;
}

const DepthPerception& OutputImageParams::depthPerception() const {
  return m_depthPerception;
}

double OutputImageParams::despeckleLevel() const {
  return m_despeckleLevel;
}

void OutputImageParams::setBlackOnWhite(bool blackOnWhite) {
  m_blackOnWhite = blackOnWhite;
}

/*=============================== PartialXform =============================*/

OutputImageParams::PartialXform::PartialXform() : m_11(), m_12(), m_21(), m_22() {}

OutputImageParams::PartialXform::PartialXform(const QTransform& xform)
    : m_11(xform.m11()), m_12(xform.m12()), m_21(xform.m21()), m_22(xform.m22()) {}

OutputImageParams::PartialXform::PartialXform(const QDomElement& el)
    : m_11(el.namedItem("m11").toElement().text().toDouble()),
      m_12(el.namedItem("m12").toElement().text().toDouble()),
      m_21(el.namedItem("m21").toElement().text().toDouble()),
      m_22(el.namedItem("m22").toElement().text().toDouble()) {}

QDomElement OutputImageParams::PartialXform::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(marshaller.string(Utils::doubleToString(m_11), "m11"));
  el.appendChild(marshaller.string(Utils::doubleToString(m_12), "m12"));
  el.appendChild(marshaller.string(Utils::doubleToString(m_21), "m21"));
  el.appendChild(marshaller.string(Utils::doubleToString(m_22), "m22"));

  return el;
}

bool OutputImageParams::PartialXform::matches(const PartialXform& other) const {
  return closeEnough(m_11, other.m_11) && closeEnough(m_12, other.m_12) && closeEnough(m_21, other.m_21)
         && closeEnough(m_22, other.m_22);
}

bool OutputImageParams::PartialXform::closeEnough(double v1, double v2) {
  return std::fabs(v1 - v2) < 0.0001;
}
}  // namespace output