
#include "DefaultParams.h"
#include "Utils.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

using namespace page_split;
using namespace output;

const DefaultParams::FixOrientationParams& DefaultParams::getFixOrientationParams() const {
  return fixOrientationParams;
}

void DefaultParams::setFixOrientationParams(const DefaultParams::FixOrientationParams& fixOrientationParams) {
  DefaultParams::fixOrientationParams = fixOrientationParams;
}

const DefaultParams::DeskewParams& DefaultParams::getDeskewParams() const {
  return deskewParams;
}

void DefaultParams::setDeskewParams(const DefaultParams::DeskewParams& deskewParams) {
  DefaultParams::deskewParams = deskewParams;
}

const DefaultParams::PageSplitParams& DefaultParams::getPageSplitParams() const {
  return pageSplitParams;
}

void DefaultParams::setPageSplitParams(const DefaultParams::PageSplitParams& pageSplitParams) {
  DefaultParams::pageSplitParams = pageSplitParams;
}

const DefaultParams::SelectContentParams& DefaultParams::getSelectContentParams() const {
  return selectContentParams;
}

void DefaultParams::setSelectContentParams(const DefaultParams::SelectContentParams& selectContentParams) {
  DefaultParams::selectContentParams = selectContentParams;
}

const DefaultParams::PageLayoutParams& DefaultParams::getPageLayoutParams() const {
  return pageLayoutParams;
}

void DefaultParams::setPageLayoutParams(const DefaultParams::PageLayoutParams& pageLayoutParams) {
  DefaultParams::pageLayoutParams = pageLayoutParams;
}

const DefaultParams::OutputParams& DefaultParams::getOutputParams() const {
  return outputParams;
}

void DefaultParams::setOutputParams(const DefaultParams::OutputParams& outputParams) {
  DefaultParams::outputParams = outputParams;
}

DefaultParams::DefaultParams(const DefaultParams::FixOrientationParams& fixOrientationParams,
                             const DefaultParams::DeskewParams& deskewParams,
                             const DefaultParams::PageSplitParams& pageSplitParams,
                             const DefaultParams::SelectContentParams& selectContentParams,
                             const DefaultParams::PageLayoutParams& pageLayoutParams,
                             const DefaultParams::OutputParams& outputParams)
    : fixOrientationParams(fixOrientationParams),
      deskewParams(deskewParams),
      pageSplitParams(pageSplitParams),
      selectContentParams(selectContentParams),
      pageLayoutParams(pageLayoutParams),
      outputParams(outputParams),
      units(MILLIMETRES) {}

DefaultParams::DefaultParams(const QDomElement& el)
    : fixOrientationParams(el.namedItem("fix-orientation-params").toElement()),
      deskewParams(el.namedItem("deskew-params").toElement()),
      pageSplitParams(el.namedItem("page-split-params").toElement()),
      selectContentParams(el.namedItem("select-content-params").toElement()),
      pageLayoutParams(el.namedItem("page-layout-params").toElement()),
      outputParams(el.namedItem("output-params").toElement()),
      units(unitsFromString(el.attribute("units"))) {}

QDomElement DefaultParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(fixOrientationParams.toXml(doc, "fix-orientation-params"));
  el.appendChild(deskewParams.toXml(doc, "deskew-params"));
  el.appendChild(pageSplitParams.toXml(doc, "page-split-params"));
  el.appendChild(selectContentParams.toXml(doc, "select-content-params"));
  el.appendChild(pageLayoutParams.toXml(doc, "page-layout-params"));
  el.appendChild(outputParams.toXml(doc, "output-params"));
  el.setAttribute("units", unitsToString(units));

  return el;
}

DefaultParams::DefaultParams() : units(MILLIMETRES) {}

Units DefaultParams::getUnits() const {
  return units;
}

void DefaultParams::setUnits(Units units) {
  DefaultParams::units = units;
}

const OrthogonalRotation& DefaultParams::FixOrientationParams::getImageRotation() const {
  return imageRotation;
}

void DefaultParams::FixOrientationParams::setImageRotation(const OrthogonalRotation& imageRotation) {
  FixOrientationParams::imageRotation = imageRotation;
}

DefaultParams::FixOrientationParams::FixOrientationParams(const OrthogonalRotation& imageRotation)
    : imageRotation(imageRotation) {}

DefaultParams::FixOrientationParams::FixOrientationParams(const QDomElement& el)
    : imageRotation(XmlUnmarshaller::rotation(el.namedItem("imageRotation").toElement())) {}

QDomElement DefaultParams::FixOrientationParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(XmlMarshaller(doc).rotation(imageRotation, "imageRotation"));

  return el;
}

DefaultParams::DeskewParams::DeskewParams(double deskewAngleDeg, AutoManualMode mode)
    : deskewAngleDeg(deskewAngleDeg), mode(mode) {}

double DefaultParams::DeskewParams::getDeskewAngleDeg() const {
  return deskewAngleDeg;
}

void DefaultParams::DeskewParams::setDeskewAngleDeg(double deskewAngleDeg) {
  DeskewParams::deskewAngleDeg = deskewAngleDeg;
}

AutoManualMode DefaultParams::DeskewParams::getMode() const {
  return mode;
}

void DefaultParams::DeskewParams::setMode(AutoManualMode mode) {
  DeskewParams::mode = mode;
}

DefaultParams::DeskewParams::DeskewParams() : deskewAngleDeg(0.0), mode(MODE_AUTO) {}

DefaultParams::DeskewParams::DeskewParams(const QDomElement& el)
    : deskewAngleDeg(el.attribute("deskewAngleDeg").toDouble()),
      mode((el.attribute("mode") == "manual") ? MODE_MANUAL : MODE_AUTO) {}

QDomElement DefaultParams::DeskewParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("deskewAngleDeg", Utils::doubleToString(deskewAngleDeg));
  el.setAttribute("mode", (mode == MODE_AUTO) ? "auto" : "manual");

  return el;
}

DefaultParams::PageSplitParams::PageSplitParams(page_split::LayoutType layoutType) : layoutType(layoutType) {}

LayoutType DefaultParams::PageSplitParams::getLayoutType() const {
  return layoutType;
}

void DefaultParams::PageSplitParams::setLayoutType(LayoutType layoutType) {
  PageSplitParams::layoutType = layoutType;
}

DefaultParams::PageSplitParams::PageSplitParams() : layoutType(AUTO_LAYOUT_TYPE) {}

DefaultParams::PageSplitParams::PageSplitParams(const QDomElement& el)
    : layoutType(layoutTypeFromString(el.attribute("layoutType"))) {}

QDomElement DefaultParams::PageSplitParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("layoutType", layoutTypeToString(layoutType));

  return el;
}

DefaultParams::SelectContentParams::SelectContentParams(const QSizeF& pageRectSize,
                                                        AutoManualMode pageDetectMode,
                                                        bool contentDetectEnabled,
                                                        bool pageDetectEnabled,
                                                        bool fineTuneCorners)
    : pageRectSize(pageRectSize),
      pageDetectMode(pageDetectMode),
      contentDetectEnabled(contentDetectEnabled),
      pageDetectEnabled(pageDetectEnabled),
      fineTuneCorners(fineTuneCorners) {}

DefaultParams::SelectContentParams::SelectContentParams()
    : pageRectSize(QSizeF(210, 297)),
      pageDetectMode(MODE_AUTO),
      contentDetectEnabled(true),
      pageDetectEnabled(false),
      fineTuneCorners(false) {}

const QSizeF& DefaultParams::SelectContentParams::getPageRectSize() const {
  return pageRectSize;
}

void DefaultParams::SelectContentParams::setPageRectSize(const QSizeF& pageRectSize) {
  SelectContentParams::pageRectSize = pageRectSize;
}

bool DefaultParams::SelectContentParams::isContentDetectEnabled() const {
  return contentDetectEnabled;
}

void DefaultParams::SelectContentParams::setContentDetectEnabled(bool contentDetectEnabled) {
  SelectContentParams::contentDetectEnabled = contentDetectEnabled;
}

bool DefaultParams::SelectContentParams::isPageDetectEnabled() const {
  return pageDetectEnabled;
}

void DefaultParams::SelectContentParams::setPageDetectEnabled(bool pageDetectEnabled) {
  SelectContentParams::pageDetectEnabled = pageDetectEnabled;
}

bool DefaultParams::SelectContentParams::isFineTuneCorners() const {
  return fineTuneCorners;
}

void DefaultParams::SelectContentParams::setFineTuneCorners(bool fineTuneCorners) {
  SelectContentParams::fineTuneCorners = fineTuneCorners;
}

DefaultParams::SelectContentParams::SelectContentParams(const QDomElement& el)
    : pageRectSize(XmlUnmarshaller::sizeF(el.namedItem("pageRectSize").toElement())),
      pageDetectMode((el.attribute("pageDetectMode") == "manual") ? MODE_MANUAL : MODE_AUTO),
      contentDetectEnabled(el.attribute("contentDetectEnabled") == "1"),
      pageDetectEnabled(el.attribute("pageDetectEnabled") == "1"),
      fineTuneCorners(el.attribute("fineTuneCorners") == "1") {}

QDomElement DefaultParams::SelectContentParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(XmlMarshaller(doc).sizeF(pageRectSize, "pageRectSize"));
  el.setAttribute("pageDetectMode", (pageDetectMode == MODE_AUTO) ? "auto" : "manual");
  el.setAttribute("contentDetectEnabled", contentDetectEnabled ? "1" : "0");
  el.setAttribute("pageDetectEnabled", pageDetectEnabled ? "1" : "0");
  el.setAttribute("fineTuneCorners", fineTuneCorners ? "1" : "0");

  return el;
}

AutoManualMode DefaultParams::SelectContentParams::getPageDetectMode() const {
  return pageDetectMode;
}

void DefaultParams::SelectContentParams::setPageDetectMode(AutoManualMode pageDetectMode) {
  SelectContentParams::pageDetectMode = pageDetectMode;
}

DefaultParams::PageLayoutParams::PageLayoutParams(const Margins& hardMargins,
                                                  const page_layout::Alignment& alignment,
                                                  bool autoMargins)
    : hardMargins(hardMargins), alignment(alignment), autoMargins(autoMargins) {}

DefaultParams::PageLayoutParams::PageLayoutParams() : hardMargins(10, 5, 10, 5), autoMargins(false) {}

const Margins& DefaultParams::PageLayoutParams::getHardMargins() const {
  return hardMargins;
}

void DefaultParams::PageLayoutParams::setHardMargins(const Margins& hardMargins) {
  PageLayoutParams::hardMargins = hardMargins;
}

const page_layout::Alignment& DefaultParams::PageLayoutParams::getAlignment() const {
  return alignment;
}

void DefaultParams::PageLayoutParams::setAlignment(const page_layout::Alignment& alignment) {
  PageLayoutParams::alignment = alignment;
}

bool DefaultParams::PageLayoutParams::isAutoMargins() const {
  return autoMargins;
}

void DefaultParams::PageLayoutParams::setAutoMargins(bool autoMargins) {
  PageLayoutParams::autoMargins = autoMargins;
}

DefaultParams::PageLayoutParams::PageLayoutParams(const QDomElement& el)
    : hardMargins(XmlUnmarshaller::margins(el.namedItem("hardMargins").toElement())),
      alignment(el.namedItem("alignment").toElement()),
      autoMargins(el.attribute("autoMargins") == "1") {}

QDomElement DefaultParams::PageLayoutParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(XmlMarshaller(doc).margins(hardMargins, "hardMargins"));
  el.appendChild(alignment.toXml(doc, "alignment"));
  el.setAttribute("autoMargins", autoMargins ? "1" : "0");

  return el;
}

DefaultParams::OutputParams::OutputParams(const Dpi& dpi,
                                          const output::ColorParams& colorParams,
                                          const output::SplittingOptions& splittingOptions,
                                          const output::PictureShapeOptions& pictureShapeOptions,
                                          const output::DepthPerception& depthPerception,
                                          const output::DewarpingOptions& dewarpingOptions,
                                          const double despeckleLevel)
    : dpi(dpi),
      colorParams(colorParams),
      splittingOptions(splittingOptions),
      pictureShapeOptions(pictureShapeOptions),
      depthPerception(depthPerception),
      dewarpingOptions(dewarpingOptions),
      despeckleLevel(despeckleLevel) {}

DefaultParams::OutputParams::OutputParams() : despeckleLevel(1.0), dpi(600, 600) {}

const Dpi& DefaultParams::OutputParams::getDpi() const {
  return dpi;
}

void DefaultParams::OutputParams::setDpi(const Dpi& dpi) {
  OutputParams::dpi = dpi;
}

const ColorParams& DefaultParams::OutputParams::getColorParams() const {
  return colorParams;
}

void DefaultParams::OutputParams::setColorParams(const ColorParams& colorParams) {
  OutputParams::colorParams = colorParams;
}

const SplittingOptions& DefaultParams::OutputParams::getSplittingOptions() const {
  return splittingOptions;
}

void DefaultParams::OutputParams::setSplittingOptions(const SplittingOptions& splittingOptions) {
  OutputParams::splittingOptions = splittingOptions;
}

const PictureShapeOptions& DefaultParams::OutputParams::getPictureShapeOptions() const {
  return pictureShapeOptions;
}

void DefaultParams::OutputParams::setPictureShapeOptions(const PictureShapeOptions& pictureShapeOptions) {
  OutputParams::pictureShapeOptions = pictureShapeOptions;
}

const DepthPerception& DefaultParams::OutputParams::getDepthPerception() const {
  return depthPerception;
}

void DefaultParams::OutputParams::setDepthPerception(const DepthPerception& depthPerception) {
  OutputParams::depthPerception = depthPerception;
}

const DewarpingOptions& DefaultParams::OutputParams::getDewarpingOptions() const {
  return dewarpingOptions;
}

void DefaultParams::OutputParams::setDewarpingOptions(const DewarpingOptions& dewarpingOptions) {
  OutputParams::dewarpingOptions = dewarpingOptions;
}

double DefaultParams::OutputParams::getDespeckleLevel() const {
  return despeckleLevel;
}

void DefaultParams::OutputParams::setDespeckleLevel(double despeckleLevel) {
  OutputParams::despeckleLevel = despeckleLevel;
}

DefaultParams::OutputParams::OutputParams(const QDomElement& el)
    : dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
      colorParams(el.namedItem("colorParams").toElement()),
      splittingOptions(el.namedItem("splittingOptions").toElement()),
      pictureShapeOptions(el.namedItem("pictureShapeOptions").toElement()),
      depthPerception(el.attribute("depthPerception").toDouble()),
      dewarpingOptions(el.namedItem("dewarpingOptions").toElement()),
      despeckleLevel(el.attribute("despeckleLevel").toDouble()) {}

QDomElement DefaultParams::OutputParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(XmlMarshaller(doc).dpi(dpi, "dpi"));
  el.appendChild(colorParams.toXml(doc, "colorParams"));
  el.appendChild(splittingOptions.toXml(doc, "splittingOptions"));
  el.appendChild(pictureShapeOptions.toXml(doc, "pictureShapeOptions"));
  el.setAttribute("depthPerception", Utils::doubleToString(depthPerception.value()));
  el.appendChild(dewarpingOptions.toXml(doc, "dewarpingOptions"));
  el.setAttribute("despeckleLevel", Utils::doubleToString(despeckleLevel));

  return el;
}
