/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef VIRTUALFUNCTION_H_
#define VIRTUALFUNCTION_H_

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


#endif  // ifndef VIRTUALFUNCTION_H_
