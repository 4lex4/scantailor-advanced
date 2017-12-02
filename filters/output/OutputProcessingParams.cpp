
#include "BlackWhiteOptions.h"
#include "OutputProcessingParams.h"
#include <QDomDocument>

namespace output {

    OutputProcessingParams::OutputProcessingParams()
            : whiteOnBlackMode(false),
              autoZonesFound(false),
              whiteOnBlackAutoDetected(false) {
    }

    OutputProcessingParams::OutputProcessingParams(QDomElement const& el)
            : whiteOnBlackMode(el.attribute("whiteOnBlackMode") == "1"),
              autoZonesFound(el.attribute("autoZonesFound") == "1"),
              whiteOnBlackAutoDetected(el.attribute("whiteOnBlackAutoDetected") == "1") {
    }

    QDomElement OutputProcessingParams::toXml(QDomDocument& doc, QString const& name) const {
        QDomElement el(doc.createElement(name));
        el.setAttribute("whiteOnBlackMode", whiteOnBlackMode ? "1" : "0");
        el.setAttribute("autoZonesFound", autoZonesFound ? "1" : "0");
        el.setAttribute("whiteOnBlackAutoDetected", whiteOnBlackAutoDetected ? "1" : "0");

        return el;
    }

    bool OutputProcessingParams::operator==(OutputProcessingParams const& other) const {
        return (whiteOnBlackMode == other.whiteOnBlackMode)
               && (autoZonesFound == other.autoZonesFound)
               && (whiteOnBlackAutoDetected == other.whiteOnBlackAutoDetected);
    }

    bool OutputProcessingParams::operator!=(OutputProcessingParams const& other) const {
        return !(*this == other);
    }

    bool output::OutputProcessingParams::isAutoZonesFound() const {
        return autoZonesFound;
    }

    void output::OutputProcessingParams::setAutoZonesFound(bool autoZonesFound) {
        OutputProcessingParams::autoZonesFound = autoZonesFound;
    }

    bool output::OutputProcessingParams::isWhiteOnBlackAutoDetected() const {
        return whiteOnBlackAutoDetected;
    }

    void output::OutputProcessingParams::setWhiteOnBlackAutoDetected(bool whiteOnBlackAutoDetected) {
        OutputProcessingParams::whiteOnBlackAutoDetected = whiteOnBlackAutoDetected;
    }

    bool output::OutputProcessingParams::isWhiteOnBlackMode() const {
        return whiteOnBlackMode;
    }

    void output::OutputProcessingParams::setWhiteOnBlackMode(bool whiteOnBlackMode) {
        OutputProcessingParams::whiteOnBlackMode = whiteOnBlackMode;
    }

}