// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_PICTURESHAPEOPTIONS_H_
#define SCANTAILOR_OUTPUT_PICTURESHAPEOPTIONS_H_

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


  PictureShape m_pictureShape;
  int m_sensitivity;
  bool m_higherSearchSensitivity;
};


inline PictureShape PictureShapeOptions::getPictureShape() const {
  return m_pictureShape;
}

inline void PictureShapeOptions::setPictureShape(PictureShape pictureShape) {
  PictureShapeOptions::m_pictureShape = pictureShape;
}

inline int PictureShapeOptions::getSensitivity() const {
  return m_sensitivity;
}

inline void PictureShapeOptions::setSensitivity(int sensitivity) {
  PictureShapeOptions::m_sensitivity = sensitivity;
}

inline bool PictureShapeOptions::isHigherSearchSensitivity() const {
  return m_higherSearchSensitivity;
}

inline void PictureShapeOptions::setHigherSearchSensitivity(bool higherSearchSensitivity) {
  PictureShapeOptions::m_higherSearchSensitivity = higherSearchSensitivity;
}
}  // namespace output

#endif  // SCANTAILOR_OUTPUT_PICTURESHAPEOPTIONS_H_
