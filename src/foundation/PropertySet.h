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

#ifndef PROPERTY_SET_H_
#define PROPERTY_SET_H_

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include "Property.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

class PropertySet : public ref_countable {
 public:
  PropertySet() = default;

  /**
   * \brief Makes a deep copy of another property set.
   */
  PropertySet(const PropertySet& other);

  PropertySet(const QDomElement& el, const PropertyFactory& factory);

  /**
   * \brief Makes a deep copy of another property set.
   */
  PropertySet& operator=(const PropertySet& other);

  void swap(PropertySet& other);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  template <typename T>
  using is_property = typename std::enable_if<std::is_base_of<Property, T>::value>::type;

  /**
   * Returns a property stored in this set, if one having a suitable
   * type is found, or returns a null smart pointer otherwise.
   */
  template <typename T, typename = is_property<T>>
  intrusive_ptr<T> locate();

  template <typename T, typename = is_property<T>>
  intrusive_ptr<const T> locate() const;

  /**
   * Returns a property stored in this set, if one having a suitable
   * type is found, or returns a default constructed object otherwise.
   */
  template <typename T, typename = is_property<T>>
  intrusive_ptr<T> locateOrDefault();

  template <typename T, typename = is_property<T>>
  intrusive_ptr<const T> locateOrDefault() const;

  /**
   * Returns a property stored in this set, if one having a suitable
   * type is found.  Otherwise, a default constructed object is put
   * to the set and then returned.
   */
  template <typename T, typename = is_property<T>>
  intrusive_ptr<T> locateOrCreate();

 private:
  using PropertyMap = std::unordered_map<std::type_index, intrusive_ptr<Property>>;

  PropertyMap m_props;
};

template <typename T, typename>
intrusive_ptr<T> PropertySet::locate() {
  auto it(m_props.find(typeid(T)));
  if (it != m_props.end()) {
    return static_pointer_cast<T>(it->second);
  } else {
    return nullptr;
  }
}

template <typename T, typename>
intrusive_ptr<const T> PropertySet::locate() const {
  return const_cast<PropertySet*>(this)->locate<T>();
}

template <typename T, typename>
intrusive_ptr<T> PropertySet::locateOrDefault() {
  intrusive_ptr<T> prop = locate<T>();
  if (!prop) {
    return make_intrusive<T>();
  }
  return prop;
}

template <typename T, typename>
intrusive_ptr<const T> PropertySet::locateOrDefault() const {
  return const_cast<PropertySet*>(this)->locateOrDefault<T>();
}

template <typename T, typename>
intrusive_ptr<T> PropertySet::locateOrCreate() {
  intrusive_ptr<T> prop = locate<T>();
  if (!prop) {
    prop = make_intrusive<T>();
    m_props[typeid(T)] = prop;
  }
  return prop;
}

inline void swap(PropertySet& o1, PropertySet& o2) {
  o1.swap(o2);
}

#endif  // ifndef PROPERTY_SET_H_
