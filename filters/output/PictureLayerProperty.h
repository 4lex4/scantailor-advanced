/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
