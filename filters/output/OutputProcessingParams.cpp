
#include "BlackWhiteOptions.h"
#include "OutputProcessingParams.h"
#include <QDomDocument>

namespace output {

OutputProcessingParams::OutputProcessingParams() : autoZonesFound(false) {
}

OutputProcessingParams::OutputProcessingParams(const QDomElement& el)
        : autoZonesFound(el.attribute("autoZonesFound") == "1") {
}

QDomElement OutputProcessingParams::toXml(QDomDocument& doc, const QString& name) const {
    QDomElement el(doc.createElement(name));
    el.setAttribute("autoZonesFound", autoZonesFound ? "1" : "0");

    return el;
}

bool OutputProcessingParams::operator==(const OutputProcessingParams& other) const {
    return (autoZonesFound == other.autoZonesFound);
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
}  // namespace output