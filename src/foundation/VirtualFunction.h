// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_VIRTUALFUNCTION_H_
#define SCANTAILOR_FOUNDATION_VIRTUALFUNCTION_H_

#include <utility>

template <typename Res, typename... ArgTypes>
class VirtualFunction {
 public:
  virtual ~VirtualFunction() {}

  virtual Res operator()(ArgTypes... args) const = 0;
};


template <typename Delegate, typename Res, typename... ArgTypes>
class ProxyFunction : public VirtualFunction<Res, ArgTypes...> {
 public:
  explicit ProxyFunction(Delegate delegate) : m_delegate(delegate) {}

  Res operator()(ArgTypes... args) const override { return m_delegate(args...); }

 private:
  Delegate m_delegate;
};


#endif  // ifndef SCANTAILOR_FOUNDATION_VIRTUALFUNCTION_H_
