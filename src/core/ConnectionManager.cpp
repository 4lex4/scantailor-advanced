// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ConnectionManager.h"

#include <QtCore/QObject>

ConnectionManager::ConnectionManager(const std::function<void()>& setupConnectionsFunc)
    : m_setupConnectionsFunc(setupConnectionsFunc) {}

void ConnectionManager::addConnection(const QMetaObject::Connection& connection) {
  m_connectionList.push_back(connection);
}

bool ConnectionManager::removeConnections() {
  if (m_connectionList.empty())
    return false;

  for (const auto& connection : m_connectionList) {
    QObject::disconnect(connection);
  }
  m_connectionList.clear();
  return true;
}

ConnectionManager::ScopedBlock ConnectionManager::getScopedBlock() {
  return ScopedBlock(*this);
}

void ConnectionManager::setupConnections() const {
  m_setupConnectionsFunc();
}

ConnectionManager::ScopedBlock::ScopedBlock(ConnectionManager& parent)
    : m_parent(parent), m_needSetup(parent.removeConnections()) {}

ConnectionManager::ScopedBlock::~ScopedBlock() {
  if (m_needSetup)
    m_parent.setupConnections();
}
