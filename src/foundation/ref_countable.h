// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_REF_COUNTABLE_H_
#define SCANTAILOR_FOUNDATION_REF_COUNTABLE_H_

#include <QAtomicInt>

class ref_countable {
 public:
  ref_countable() = default;

  ref_countable(const ref_countable&) {}

  ref_countable& operator=(const ref_countable&) { return *this; }

  void ref() const { m_counter.fetchAndAddRelaxed(1); }

  void unref() const {
    if (m_counter.fetchAndAddRelease(-1) == 1) {
      delete this;
    }
  }

 protected:
  virtual ~ref_countable() = default;

 private:
  mutable QAtomicInt m_counter = 0;
};


#endif  // ifndef SCANTAILOR_FOUNDATION_REF_COUNTABLE_H_
