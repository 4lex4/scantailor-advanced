// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DefaultParams.h"

#include <foundation/Utils.h>

#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

using namespace foundation;
using namespace page_split;
using namespace output;

const DefaultParams::FixOrientationParams& DefaultParams::getFixOrientationParams() const {
  return m_fixOrientationParams;
}

void DefaultParams::setFixOrientationParams(const DefaultParams::FixOrientationParams& fixOrientationParams) {
  DefaultParams::m_fixOrientationParams = fixOrientationParams;
}

const DefaultParams::DeskewParams& DefaultParams::getDeskewParams() const {
  return m_deskewParams;
}

void DefaultParams::setDeskewParams(const DefaultParams::DeskewParams& deskewParams) {
  DefaultParams::m_deskewParams = deskewParams;
}

const DefaultParams::PageSplitParams& DefaultParams::getPageSplitParams() const {
  return m_pageSplitParams;
}

void DefaultParams::setPageSplitParams(const DefaultParams::PageSplitParams& pageSplitParams) {
  DefaultParams::m_pageSplitParams = pageSplitParams;
}

const DefaultParams::SelectContentParams& DefaultParams::getSelectContentParams() const {
  return m_selectContentParams;
}

void DefaultParams::setSelectContentParams(const DefaultParams::SelectContentParams& selectContentParams) {
  DefaultParams::m_selectContentParams = selectContentParams;
}

const DefaultParams::PageLayoutParams& DefaultParams::getPageLayoutParams() const {
  return m_pageLayoutParams;
}

void DefaultParams::setPageLayoutParams(const DefaultParams::PageLayoutParams& pageLayoutParams) {
  DefaultParams::m_pageLayoutParams = pageLayoutParams;
}

const DefaultParams::OutputParams& DefaultParams::getOutputParams() const {
  return m_outputParams;
}

void DefaultParams::setOutputParams(const DefaultParams::OutputParams& outputParams) {
  DefaultParams::m_outputParams = outputParams;
}

DefaultParams::DefaultParams(const DefaultParams::FixOrientationParams& fixOrientationParams,
                             const DefaultParams::DeskewParams& deskewParams,
                             const DefaultParams::PageSplitParams& pageSplitParams,
                             const DefaultParams::SelectContentParams& selectContentParams,
                             const DefaultParams::PageLayoutParams& pageLayoutParams,
                             const DefaultParams::OutputParams& outputParams)
    : m_fixOrientationParams(fixOrientationParams),
      m_deskewParams(deskewParams),
      m_pageSplitParams(pageSplitParams),
      m_selectContentParams(selectContentParams),
      m_pageLayoutParams(pageLayoutParams),
      m_outputParams(outputParams),
      m_units(MILLIMETRES) {}

DefaultParams::DefaultParams(const QDomElement& el)
    : m_fixOrientationParams(el.namedItem("fix-orientation-params").toElement()),
      m_deskewParams(el.namedItem("deskew-params").toElement()),
      m_pageSplitParams(el.namedItem("page-split-params").toElement()),
      m_selectContentParams(el.namedItem("select-content-params").toElement()),
      m_pageLayoutParams(el.namedItem("page-layout-params").toElement()),
      m_outputParams(el.namedItem("output-params").toElement()),
      m_units(unitsFromString(el.attribute("units"))) {}

QDomElement DefaultParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(m_fixOrientationParams.toXml(doc, "fix-orientation-params"));
  el.appendChild(m_deskewParams.toXml(doc, "deskew-params"));
  el.appendChild(m_pageSplitParams.toXml(doc, "page-split-params"));
  el.appendChild(m_selectContentParams.toXml(doc, "select-content-params"));
  el.appendChild(m_pageLayoutParams.toXml(doc, "page-layout-params"));
  el.appendChild(m_outputParams.toXml(doc, "output-params"));
  el.setAttribute("units", unitsToString(m_units));
  return el;
}

DefaultParams::DefaultParams() : m_units(MILLIMETRES) {}

Units DefaultParams::getUnits() const {
  return m_units;
}

void DefaultParams::setUnits(Units units) {
  DefaultParams::m_units = units;
}

const OrthogonalRotation& DefaultParams::FixOrientationParams::getImageRotation() const {
  return m_imageRotation;
}

void DefaultParams::FixOrientationParams::setImageRotation(const OrthogonalRotation& imageRotation) {
  FixOrientationParams::m_imageRotation = imageRotation;
}

DefaultParams::FixOrientationParams::FixOrientationParams(const OrthogonalRotation& imageRotation)
    : m_imageRotation(imageRotation) {}

DefaultParams::FixOrientationParams::FixOrientationParams(const QDomElement& el)
    : m_imageRotation(el.namedItem("imageRotation").toElement()) {}

QDomElement DefaultParams::FixOrientationParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(m_imageRotation.toXml(doc, "imageRotation"));
  return el;
}

DefaultParams::DeskewParams::DeskewParams(double deskewAngleDeg, AutoManualMode mode)
    : m_deskewAngleDeg(deskewAngleDeg), m_mode(mode) {}

double DefaultParams::DeskewParams::getDeskewAngleDeg() const {
  return m_deskewAngleDeg;
}

void DefaultParams::DeskewParams::setDeskewAngleDeg(double deskewAngleDeg) {
  DeskewParams::m_deskewAngleDeg = deskewAngleDeg;
}

AutoManualMode DefaultParams::DeskewParams::getMode() const {
  return m_mode;
}

void DefaultParams::DeskewParams::setMode(AutoManualMode mode) {
  DeskewParams::m_mode = mode;
}

DefaultParams::DeskewParams::DeskewParams() : m_deskewAngleDeg(0.0), m_mode(MODE_AUTO) {}

DefaultParams::DeskewParams::DeskewParams(const QDomElement& el)
    : m_deskewAngleDeg(el.attribute("deskewAngleDeg").toDouble()),
      m_mode((el.attribute("mode") == "manual") ? MODE_MANUAL : MODE_AUTO) {}

QDomElement DefaultParams::DeskewParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("deskewAngleDeg", Utils::doubleToString(m_deskewAngleDeg));
  el.setAttribute("mode", (m_mode == MODE_AUTO) ? "auto" : "manual");
  return el;
}

DefaultParams::PageSplitParams::PageSplitParams(page_split::LayoutType layoutType) : m_layoutType(layoutType) {}

LayoutType DefaultParams::PageSplitParams::getLayoutType() const {
  return m_layoutType;
}

void DefaultParams::PageSplitParams::setLayoutType(LayoutType layoutType) {
  PageSplitParams::m_layoutType = layoutType;
}

DefaultParams::PageSplitParams::PageSplitParams() : m_layoutType(AUTO_LAYOUT_TYPE) {}

DefaultParams::PageSplitParams::PageSplitParams(const QDomElement& el)
    : m_layoutType(layoutTypeFromString(el.attribute("layoutType"))) {}

QDomElement DefaultParams::PageSplitParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("layoutType", layoutTypeToString(m_layoutType));
  return el;
}

DefaultParams::SelectContentParams::SelectContentParams(const QSizeF& pageRectSize,
                                                        bool contentDetectEnabled,
                                                        AutoManualMode pageDetectMode,
                                                        bool fineTuneCorners)
    : m_pageRectSize(pageRectSize),
      m_contentDetectEnabled(contentDetectEnabled),
      m_pageDetectMode(pageDetectMode),
      m_fineTuneCorners(fineTuneCorners) {}

DefaultParams::SelectContentParams::SelectContentParams()
    : m_pageRectSize(QSizeF(210, 297)),
      m_contentDetectEnabled(true),
      m_pageDetectMode(MODE_DISABLED),
      m_fineTuneCorners(false) {}

const QSizeF& DefaultParams::SelectContentParams::getPageRectSize() const {
  return m_pageRectSize;
}

void DefaultParams::SelectContentParams::setPageRectSize(const QSizeF& pageRectSize) {
  SelectContentParams::m_pageRectSize = pageRectSize;
}

bool DefaultParams::SelectContentParams::isContentDetectEnabled() const {
  return m_contentDetectEnabled;
}

void DefaultParams::SelectContentParams::setContentDetectEnabled(bool contentDetectEnabled) {
  SelectContentParams::m_contentDetectEnabled = contentDetectEnabled;
}

bool DefaultParams::SelectContentParams::isFineTuneCorners() const {
  return m_fineTuneCorners;
}

void DefaultParams::SelectContentParams::setFineTuneCorners(bool fineTuneCorners) {
  SelectContentParams::m_fineTuneCorners = fineTuneCorners;
}

DefaultParams::SelectContentParams::SelectContentParams(const QDomElement& el)
    : m_pageRectSize(XmlUnmarshaller::sizeF(el.namedItem("pageRectSize").toElement())),
      m_contentDetectEnabled(el.attribute("contentDetectEnabled") == "1"),
      m_pageDetectMode(stringToAutoManualMode(el.attribute("pageDetectMode"))),
      m_fineTuneCorners(el.attribute("fineTuneCorners") == "1") {}

QDomElement DefaultParams::SelectContentParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(XmlMarshaller(doc).sizeF(m_pageRectSize, "pageRectSize"));
  el.setAttribute("contentDetectEnabled", m_contentDetectEnabled ? "1" : "0");
  el.setAttribute("pageDetectMode", autoManualModeToString(m_pageDetectMode));
  el.setAttribute("fineTuneCorners", m_fineTuneCorners ? "1" : "0");
  return el;
}

AutoManualMode DefaultParams::SelectContentParams::getPageDetectMode() const {
  return m_pageDetectMode;
}

void DefaultParams::SelectContentParams::setPageDetectMode(AutoManualMode pageDetectMode) {
  SelectContentParams::m_pageDetectMode = pageDetectMode;
}

DefaultParams::PageLayoutParams::PageLayoutParams(const Margins& hardMargins,
                                                  const page_layout::Alignment& alignment,
                                                  bool autoMargins)
    : m_hardMargins(hardMargins), m_alignment(alignment), m_autoMargins(autoMargins) {}

DefaultParams::PageLayoutParams::PageLayoutParams() : m_hardMargins(10, 5, 10, 5), m_autoMargins(false) {}

const Margins& DefaultParams::PageLayoutParams::getHardMargins() const {
  return m_hardMargins;
}

void DefaultParams::PageLayoutParams::setHardMargins(const Margins& hardMargins) {
  PageLayoutParams::m_hardMargins = hardMargins;
}

const page_layout::Alignment& DefaultParams::PageLayoutParams::getAlignment() const {
  return m_alignment;
}

void DefaultParams::PageLayoutParams::setAlignment(const page_layout::Alignment& alignment) {
  PageLayoutParams::m_alignment = alignment;
}

bool DefaultParams::PageLayoutParams::isAutoMargins() const {
  return m_autoMargins;
}

void DefaultParams::PageLayoutParams::setAutoMargins(bool autoMargins) {
  PageLayoutParams::m_autoMargins = autoMargins;
}

DefaultParams::PageLayoutParams::PageLayoutParams(const QDomElement& el)
    : m_hardMargins(el.namedItem("hardMargins").toElement()),
      m_alignment(el.namedItem("alignment").toElement()),
      m_autoMargins(el.attribute("autoMargins") == "1") {}

QDomElement DefaultParams::PageLayoutParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(m_hardMargins.toXml(doc, "hardMargins"));
  el.appendChild(m_alignment.toXml(doc, "alignment"));
  el.setAttribute("autoMargins", m_autoMargins ? "1" : "0");
  return el;
}

DefaultParams::OutputParams::OutputParams(const Dpi& dpi,
                                          const output::ColorParams& colorParams,
                                          const output::SplittingOptions& splittingOptions,
                                          const output::PictureShapeOptions& pictureShapeOptions,
                                          const output::DepthPerception& depthPerception,
                                          const output::DewarpingOptions& dewarpingOptions,
                                          const double despeckleLevel)
    : m_dpi(dpi),
      m_colorParams(colorParams),
      m_splittingOptions(splittingOptions),
      m_pictureShapeOptions(pictureShapeOptions),
      m_depthPerception(depthPerception),
      m_dewarpingOptions(dewarpingOptions),
      m_despeckleLevel(despeckleLevel) {}

DefaultParams::OutputParams::OutputParams() : m_dpi(600, 600), m_despeckleLevel(1.0) {}

const Dpi& DefaultParams::OutputParams::getDpi() const {
  return m_dpi;
}

void DefaultParams::OutputParams::setDpi(const Dpi& dpi) {
  OutputParams::m_dpi = dpi;
}

const ColorParams& DefaultParams::OutputParams::getColorParams() const {
  return m_colorParams;
}

void DefaultParams::OutputParams::setColorParams(const ColorParams& colorParams) {
  OutputParams::m_colorParams = colorParams;
}

const SplittingOptions& DefaultParams::OutputParams::getSplittingOptions() const {
  return m_splittingOptions;
}

void DefaultParams::OutputParams::setSplittingOptions(const SplittingOptions& splittingOptions) {
  OutputParams::m_splittingOptions = splittingOptions;
}

const PictureShapeOptions& DefaultParams::OutputParams::getPictureShapeOptions() const {
  return m_pictureShapeOptions;
}

void DefaultParams::OutputParams::setPictureShapeOptions(const PictureShapeOptions& pictureShapeOptions) {
  OutputParams::m_pictureShapeOptions = pictureShapeOptions;
}

const DepthPerception& DefaultParams::OutputParams::getDepthPerception() const {
  return m_depthPerception;
}

void DefaultParams::OutputParams::setDepthPerception(const DepthPerception& depthPerception) {
  OutputParams::m_depthPerception = depthPerception;
}

const DewarpingOptions& DefaultParams::OutputParams::getDewarpingOptions() const {
  return m_dewarpingOptions;
}

void DefaultParams::OutputParams::setDewarpingOptions(const DewarpingOptions& dewarpingOptions) {
  OutputParams::m_dewarpingOptions = dewarpingOptions;
}

double DefaultParams::OutputParams::getDespeckleLevel() const {
  return m_despeckleLevel;
}

void DefaultParams::OutputParams::setDespeckleLevel(double despeckleLevel) {
  OutputParams::m_despeckleLevel = despeckleLevel;
}

DefaultParams::OutputParams::OutputParams(const QDomElement& el)
    : m_dpi(el.namedItem("dpi").toElement()),
      m_colorParams(el.namedItem("colorParams").toElement()),
      m_splittingOptions(el.namedItem("splittingOptions").toElement()),
      m_pictureShapeOptions(el.namedItem("pictureShapeOptions").toElement()),
      m_depthPerception(el.attribute("depthPerception").toDouble()),
      m_dewarpingOptions(el.namedItem("dewarpingOptions").toElement()),
      m_despeckleLevel(el.attribute("despeckleLevel").toDouble()) {}

QDomElement DefaultParams::OutputParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(m_dpi.toXml(doc, "dpi"));
  el.appendChild(m_colorParams.toXml(doc, "colorParams"));
  el.appendChild(m_splittingOptions.toXml(doc, "splittingOptions"));
  el.appendChild(m_pictureShapeOptions.toXml(doc, "pictureShapeOptions"));
  el.setAttribute("depthPerception", Utils::doubleToString(m_depthPerception.value()));
  el.appendChild(m_dewarpingOptions.toXml(doc, "dewarpingOptions"));
  el.setAttribute("despeckleLevel", Utils::doubleToString(m_despeckleLevel));
  return el;
}
