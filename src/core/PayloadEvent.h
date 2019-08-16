// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAYLOAD_EVENT_H_
#define PAYLOAD_EVENT_H_

#include <QEvent>

template <typename T>
class PayloadEvent : public QEvent {
 public:
  explicit PayloadEvent(const T& payload) : QEvent(User), m_payload(payload) {}

  const T& payload() const { return m_payload; }

  T& payload() { return m_payload; }

 private:
  T m_payload;
};


#endif
