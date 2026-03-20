// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_PICTURELAYERPROPERTY_H_
#define SCANTAILOR_OUTPUT_PICTURELAYERPROPERTY_H_

#include <memory>

#include "Property.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output {
class PictureLayerProperty : public Property {
 public:
  enum Layer { ZONENOOP, ZONEERASER1, ZONEPAINTER2, ZONEERASER3, ZONEFG, ZONEBG };

  explicit PictureLayerProperty(Layer layer = ZONENOOP);

  explicit PictureLayerProperty(const QDomElement& el);

  static void registerIn(PropertyFactory& factory);

  std::shared_ptr<Property> clone() const override;

  QDomElement toXml(QDomDocument& doc, const QString& name) const override;

  Layer layer() const;

  void setLayer(Layer layer);

 private:
  static std::shared_ptr<Property> construct(const QDomElement& el);

  static Layer layerFromString(const QString& str);

  static QString layerToString(Layer layer);


  static const char m_propertyName[];
  Layer m_layer;
};


inline PictureLayerProperty::Layer PictureLayerProperty::layer() const {
  return m_layer;
}

inline void PictureLayerProperty::setLayer(PictureLayerProperty::Layer layer) {
  m_layer = layer;
}
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_PICTURELAYERPROPERTY_H_
