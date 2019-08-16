// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_PICTURE_LAYER_PROPERTY_H_
#define OUTPUT_PICTURE_LAYER_PROPERTY_H_

#include "Property.h"
#include "intrusive_ptr.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output {
class PictureLayerProperty : public Property {
 public:
  enum Layer { NO_OP, ERASER1, PAINTER2, ERASER3 };

  explicit PictureLayerProperty(Layer layer = NO_OP);

  explicit PictureLayerProperty(const QDomElement& el);

  static void registerIn(PropertyFactory& factory);

  intrusive_ptr<Property> clone() const override;

  QDomElement toXml(QDomDocument& doc, const QString& name) const override;

  Layer layer() const;

  void setLayer(Layer layer);

 private:
  static intrusive_ptr<Property> construct(const QDomElement& el);

  static Layer layerFromString(const QString& str);

  static QString layerToString(Layer layer);


  static const char m_propertyName[];
  Layer m_layer;
};
}  // namespace output
#endif  // ifndef OUTPUT_PICTURE_LAYER_PROPERTY_H_
