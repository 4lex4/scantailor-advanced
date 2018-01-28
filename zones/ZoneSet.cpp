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

#include "ZoneSet.h"
#include <QDomNode>

ZoneSet::ZoneSet(const QDomElement& el, const PropertyFactory& prop_factory) {
    const QString zone_str("zone");

    QDomNode node(el.firstChild());
    for (; !node.isNull(); node = node.nextSibling()) {
        if (!node.isElement()) {
            continue;
        }
        if (node.nodeName() != zone_str) {
            continue;
        }

        const Zone zone(node.toElement(), prop_factory);
        if (zone.isValid()) {
            m_zones.push_back(zone);
        }
    }
}

QDomElement ZoneSet::toXml(QDomDocument& doc, const QString& name) const {
    const QString zone_str("zone");

    QDomElement el(doc.createElement(name));
    for (const Zone& zone : m_zones) {
        el.appendChild(zone.toXml(doc, zone_str));
    }

    return el;
}

void ZoneSet::applyToZoneSet(const std::function<bool(const Zone& zone)>& predicate,
                             const std::function<void(std::list<Zone>& zones,
                                                      const std::list<Zone>::iterator& iter)>& consumer) {
    for (auto it = m_zones.begin(); it != m_zones.end();) {
        if (predicate(*it)) {
            consumer(m_zones, it);
        }
        it++;
    }
}

