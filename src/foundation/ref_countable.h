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

#ifndef SCANTAILOR_REF_COUNTABLE_H
#define SCANTAILOR_REF_COUNTABLE_H

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


#endif  // ifndef SCANTAILOR_REF_COUNTABLE_H
