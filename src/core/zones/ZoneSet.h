// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ZONE_SET_H_
#define ZONE_SET_H_

#include <boost/iterator/iterator_facade.hpp>
#include <list>
#include "Zone.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

class ZoneSet {
 public:
  typedef std::list<Zone>::const_iterator const_iterator;

  ZoneSet() = default;

  ZoneSet(const QDomElement& el, const PropertyFactory& prop_factory);

  virtual ~ZoneSet() = default;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool empty() const { return m_zones.empty(); }

  void add(const Zone& zone) { m_zones.push_back(zone); }

  const_iterator erase(const_iterator position) { return m_zones.erase(position); }

  const_iterator begin() const { return m_zones.begin(); }

  const_iterator end() const { return m_zones.end(); }

 private:
  std::list<Zone> m_zones;
};


#endif  // ifndef ZONE_SET_H_
