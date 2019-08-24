// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_EDITABLEZONESET_H_
#define SCANTAILOR_ZONES_EDITABLEZONESET_H_

#include <QObject>
#include <boost/foreach.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include "EditableSpline.h"
#include "PropertySet.h"
#include "intrusive_ptr.h"

class EditableZoneSet : public QObject {
  Q_OBJECT
 private:
  using ZoneItem = std::pair<EditableSpline::Ptr, intrusive_ptr<PropertySet>>;

  class ZoneItemOrderedTag;

  using ZoneItems = boost::multi_index_container<
      ZoneItem,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<boost::multi_index::member<ZoneItem, EditableSpline::Ptr, &ZoneItem::first>,
                                            EditableSpline::Ptr::hash>,
          boost::multi_index::sequenced<boost::multi_index::tag<ZoneItemOrderedTag>>>>;

  using ZoneItemsInOrder = ZoneItems::index<ZoneItemOrderedTag>::type;

 public:
  class const_iterator;

  class Zone {
    friend class EditableZoneSet::const_iterator;

   public:
    Zone() = default;

    const EditableSpline::Ptr& spline() const { return m_iter->first; }

    const intrusive_ptr<PropertySet>& properties() const { return m_iter->second; }

   private:
    explicit Zone(ZoneItemsInOrder::const_iterator it) : m_iter(it) {}

    ZoneItemsInOrder::const_iterator m_iter;
  };


  class const_iterator : public boost::iterator_facade<const_iterator, const Zone, boost::bidirectional_traversal_tag> {
    friend class EditableZoneSet;

   public:
    const_iterator() : m_zone() {}

    void increment() { ++m_zone.m_iter; }

    void decrement() { --m_zone.m_iter; }

    bool equal(const const_iterator& other) const { return m_zone.m_iter == other.m_zone.m_iter; }

    const Zone& dereference() const { return m_zone; }

   private:
    explicit const_iterator(ZoneItemsInOrder::const_iterator it) : m_zone(it) {}

    Zone m_zone;
  };

  using iterator = const_iterator;

  EditableZoneSet();

  const_iterator begin() const { return iterator(m_zoneItemsInOrder.begin()); }

  const_iterator end() const { return iterator(m_zoneItemsInOrder.end()); }

  const PropertySet& defaultProperties() const { return m_defaultProps; }

  void setDefaultProperties(const PropertySet& props);

  void addZone(const EditableSpline::Ptr& spline);

  void addZone(const EditableSpline::Ptr& spline, const PropertySet& props);

  void removeZone(const EditableSpline::Ptr& spline);

  void commit();

  intrusive_ptr<PropertySet> propertiesFor(const EditableSpline::Ptr& spline);

  intrusive_ptr<const PropertySet> propertiesFor(const EditableSpline::Ptr& spline) const;

 signals:

  void committed();

 private:
  ZoneItems m_zoneItems;
  ZoneItemsInOrder& m_zoneItemsInOrder;
  PropertySet m_defaultProps;
};

#endif  // ifndef SCANTAILOR_ZONES_EDITABLEZONESET_H_
