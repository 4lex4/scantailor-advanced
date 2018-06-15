
#ifndef SCANTAILOR_PICTURESHAPEOPTIONS_H
#define SCANTAILOR_PICTURESHAPEOPTIONS_H

class QString;
class QDomDocument;
class QDomElement;

namespace output {
enum PictureShape { OFF_SHAPE, FREE_SHAPE, RECTANGULAR_SHAPE };

class PictureShapeOptions {
 public:
  PictureShapeOptions();

  explicit PictureShapeOptions(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool operator==(const PictureShapeOptions& other) const;

  bool operator!=(const PictureShapeOptions& other) const;

  PictureShape getPictureShape() const;

  void setPictureShape(PictureShape pictureShape);

  int getSensitivity() const;

  void setSensitivity(int sensitivity);

  bool isHigherSearchSensitivity() const;

  void setHigherSearchSensitivity(bool higherSearchSensitivity);

 private:
  static PictureShape parsePictureShape(const QString& str);

  static QString formatPictureShape(PictureShape type);


  PictureShape pictureShape;
  int sensitivity;
  bool higherSearchSensitivity;
};
}  // namespace output

#endif  // SCANTAILOR_PICTURESHAPEOPTIONS_H
