
#ifndef SCANTAILOR_OUTPUTPROCESSINGPARAMS_H
#define SCANTAILOR_OUTPUTPROCESSINGPARAMS_H

class QString;
class QDomDocument;
class QDomElement;

namespace output {
class OutputProcessingParams {
public:
    OutputProcessingParams();

    explicit OutputProcessingParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    bool operator==(const OutputProcessingParams& other) const;

    bool operator!=(const OutputProcessingParams& other) const;

    bool isAutoZonesFound() const;

    void setAutoZonesFound(bool autoZonesFound);

private:
    bool autoZonesFound;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUTPROCESSINGPARAMS_H
