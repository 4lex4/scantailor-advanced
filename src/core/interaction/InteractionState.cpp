// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "InteractionState.h"

InteractionState::Captor& InteractionState::Captor::operator=(Captor& other) {
  swap_nodes(other);
  other.unlink();
  return *this;
}

InteractionState::Captor& InteractionState::Captor::operator=(CopyHelper other) {
  return (*this = *other.captor);
}

InteractionState::InteractionState()
    : m_proximityThreshold(Proximity::fromDist(10.0)),
      m_bestProximityPriority(std::numeric_limits<int>::min()),
      m_redrawRequested(false) {}

void InteractionState::capture(Captor& captor) {
  captor.unlink();
  m_captorList.push_back(captor);
}

bool InteractionState::capturedBy(const Captor& captor) const {
  return !m_captorList.empty() && &m_captorList.back() == &captor;
}

void InteractionState::resetProximity() {
  m_proximityLeader.clear();
  m_bestProximity = Proximity();
  m_bestProximityPriority = std::numeric_limits<int>::min();
}

void InteractionState::updateProximity(Captor& captor,
                                       const Proximity& proximity,
                                       int priority,
                                       Proximity proximityThreshold) {
  if (captor.is_linked()) {
    return;
  }

  if (proximityThreshold == Proximity()) {
    proximityThreshold = m_proximityThreshold;
  }

  if (proximity <= proximityThreshold) {
    if (betterProximity(proximity, priority)) {
      m_proximityLeader.clear();
      m_proximityLeader.push_back(captor);
      m_bestProximity = proximity;
      m_bestProximityPriority = priority;
    }
  }
}

bool InteractionState::proximityLeader(const Captor& captor) const {
  return !m_proximityLeader.empty() && &m_proximityLeader.front() == &captor;
}

bool InteractionState::betterProximity(const Proximity& proximity, const int priority) const {
  if (priority != m_bestProximityPriority) {
    return priority > m_bestProximityPriority;
  }
  return proximity < m_bestProximity;
}

QCursor InteractionState::cursor() const {
  if (!m_captorList.empty()) {
    return m_captorList.back().interactionCursor();
  } else if (!m_proximityLeader.empty()) {
    return m_proximityLeader.front().proximityCursor();
  } else {
    return QCursor();
  }
}

QString InteractionState::statusTip() const {
  if (!m_captorList.empty()) {
    return m_captorList.back().interactionOrProximityStatusTip();
  } else if (!m_proximityLeader.empty()) {
    return m_proximityLeader.front().proximityStatusTip();
  } else {
    return m_defaultStatusTip;
  }
}
