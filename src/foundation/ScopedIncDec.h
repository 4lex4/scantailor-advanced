// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCOPEDINCDEC_H_
#define SCOPEDINCDEC_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

template <class T>
class ScopedIncDec {
 public:
  explicit ScopedIncDec(T& counter) : m_counter(counter) { ++counter; }

  ~ScopedIncDec() { --m_counter; }

 private:
  T& m_counter;
};


#endif
