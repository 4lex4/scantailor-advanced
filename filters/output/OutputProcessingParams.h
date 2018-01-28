
#ifndef SCANTAILOR_OUTPUTPROCESSINGPARAMS_H
#define SCANTAILOR_OUTPUTPROCESSINGPARAMS_H

class QString;
class QDomDocument;
class QDomElement;

namespace output {
    class OutputProcessingParams {
    public:
        OutputProcessingParams();

        explicit OutputProcessingParams(QDomElement const& el);

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

        bool operator==(OutputProcessingParams const& other) const;

        bool operator!=(OutputProcessingParams const& other) const;

        bool isWhiteOnBlackAutoDetected() const;

        void setWhiteOnBlackAutoDetected(bool whiteOnBlackAutoDetected);

        bool isAutoZonesFound() const;

        void setAutoZonesFound(bool autoZonesFound);

        bool isWhiteOnBlackMode() const;

        void setWhiteOnBlackMode(bool whiteOnBlackMode);

    private:
        bool whiteOnBlackMode;
        bool autoZonesFound;
        bool whiteOnBlackAutoDetected;
    };
}


#endif //SCANTAILOR_OUTPUTPROCESSINGPARAMS_H
