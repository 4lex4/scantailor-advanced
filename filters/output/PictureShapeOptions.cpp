
#include "PictureShapeOptions.h"
#include <QDomDocument>

namespace output {
    PictureShapeOptions::PictureShapeOptions()
            : pictureShape(FREE_SHAPE),
              sensitivity(100),
              autoZonesFound(false) {
    }

    PictureShapeOptions::PictureShapeOptions(QDomElement const& el)
            : pictureShape(parsePictureShape(el.attribute("pictureShape"))),
              sensitivity(el.attribute("sensitivity").toInt()),
              autoZonesFound(el.attribute("autoZonesFound") == "1") {
    }

    QDomElement PictureShapeOptions::toXml(QDomDocument& doc, QString const& name) const {
        QDomElement el(doc.createElement(name));
        el.setAttribute("pictureShape", formatPictureShape(pictureShape));
        el.setAttribute("sensitivity", sensitivity);
        el.setAttribute("autoZonesFound", autoZonesFound ? "1" : "0");

        return el;
    }

    bool PictureShapeOptions::operator==(PictureShapeOptions const& other) const {
        return (pictureShape == other.pictureShape)
               && (sensitivity == other.sensitivity);
    }

    bool PictureShapeOptions::operator!=(PictureShapeOptions const& other) const {
        return !(*this == other);
    }

    PictureShape PictureShapeOptions::parsePictureShape(const QString& str) {
        if (str == "rectangular") {
            return RECTANGULAR_SHAPE;
        } else {
            return FREE_SHAPE;
        }
    }

    QString PictureShapeOptions::formatPictureShape(PictureShape type) {
        QString str = "";
        switch (type) {
            case FREE_SHAPE:
                str = "free";
                break;
            case RECTANGULAR_SHAPE:
                str = "rectangular";
                break;
        }

        return str;
    }

    PictureShape PictureShapeOptions::getPictureShape() const {
        return pictureShape;
    }

    void PictureShapeOptions::setPictureShape(PictureShape pictureShape) {
        PictureShapeOptions::pictureShape = pictureShape;
    }

    int PictureShapeOptions::getSensitivity() const {
        return sensitivity;
    }

    void PictureShapeOptions::setSensitivity(int sensitivity) {
        PictureShapeOptions::sensitivity = sensitivity;
    }

    bool PictureShapeOptions::isAutoZonesFound() const {
        return autoZonesFound;
    }

    void PictureShapeOptions::setAutoZonesFound(bool autoZonesFound) {
        PictureShapeOptions::autoZonesFound = autoZonesFound;
    }
}
