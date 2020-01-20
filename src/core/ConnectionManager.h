// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_CONNECTIONMANAGER_H_
#define SCANTAILOR_CORE_CONNECTIONMANAGER_H_

#include <QtCore/QArgument>
#include <functional>
#include <list>

class ConnectionManager {
 public:
  class ScopedBlock {
   public:
    explicit ScopedBlock(ConnectionManager& parent);

    ~ScopedBlock();

   private:
    ConnectionManager& m_parent;
    bool m_needSetup;
  };

  explicit ConnectionManager(const std::function<void()>& setupConnectionsFunc);

  void addConnection(const QMetaObject::Connection& connection);

  ScopedBlock getScopedBlock();

  void setupConnections() const;

 private:
  bool removeConnections();

  std::function<void()> m_setupConnectionsFunc;
  std::list<QMetaObject::Connection> m_connectionList;
};


#endif  // SCANTAILOR_CORE_CONNECTIONMANAGER_H_
