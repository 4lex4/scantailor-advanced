// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_ZONECONTEXTMENUITEM_H_
#define SCANTAILOR_ZONES_ZONECONTEXTMENUITEM_H_

#include <QString>
#include <boost/function.hpp>

class InteractionState;
class InteractionHandler;

class ZoneContextMenuItem {
 public:
  /**
   * A callback may either return the InteractionHandler to connect
   * in place of ZoneContextMenuInteraction, or null, indicating not
   * to connect anything when ZoneContextMenuInteraction is disconnected.
   * The ownership of the returned InteractionHandler will be transferred
   * to ZoneInteractionContext.  It's still possible for the returned
   * InteractionHandler to be a member of another object, but in this case
   * you will need to make sure it's disconnected from ZoneInteractionContext
   * before ZoneInteractionContext destroys.
   */
  using Callback = boost::function<InteractionHandler*(InteractionState&)>;

  ZoneContextMenuItem(const QString& label, const Callback& callback) : m_label(label), m_callback(callback) {}

  const QString& label() const { return m_label; }

  const Callback& callback() const { return m_callback; }

 private:
  QString m_label;
  Callback m_callback;
};


#endif  // ifndef SCANTAILOR_ZONES_ZONECONTEXTMENUITEM_H_
