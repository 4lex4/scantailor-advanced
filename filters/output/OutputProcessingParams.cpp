
#include "OutputProcessingParams.h"
#include <QDomDocument>
#include "BlackWhiteOptions.h"

namespace output {

OutputProcessingParams::OutputProcessingParams() : autoZonesFound(false), blackOnWhiteSetManually(false) {}

OutputProcessingParams::OutputProcessingParams(const QDomElement& el)
    : autoZonesFound(el.attribute("autoZonesFound") == "1"),
      blackOnWhiteSetManually(el.attribute("blackOnWhiteSetManually") == "1") {}

QDomElement OutputProcessingParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("autoZonesFound", autoZonesFound ? "1" : "0");
  el.setAttribute("blackOnWhiteSetManually", blackOnWhiteSetManually ? "1" : "0");

  return el;
}

bool OutputProcessingParams::operator==(const OutputProcessingParams& other) const {
  return (autoZonesFound == other.autoZonesFound) && (blackOnWhiteSetManually == other.blackOnWhiteSetManually);
}

bool OutputProcessingParams::operator!=(const OutputProcessingParams& other) const {
  return !(*this == other);
}

bool output::OutputProcessingParams::isAutoZonesFound() const {
  return autoZonesFound;
}

void output::OutputProcessingParams::setAutoZonesFound(bool autoZonesFound) {
  OutputProcessingParams::autoZonesFound = autoZonesFound;
}

bool OutputProcessingParams::isBlackOnWhiteSetManually() const {
  return blackOnWhiteSetManually;
}

void OutputProcessingParams::setBlackOnWhiteSetManually(bool blackOnWhiteSetManually) {
  OutputProcessingParams::blackOnWhiteSetManually = blackOnWhiteSetManually;
}
}  // namespace output