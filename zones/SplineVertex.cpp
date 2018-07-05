/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "SplineVertex.h"
#include <cassert>


/*============================= SplineVertex ============================*/

SplineVertex::SplineVertex(SplineVertex* prev, SplineVertex* next) : m_prev(prev), m_next(next) {}

void SplineVertex::remove() {
  // Be very careful here - don't let this object
  // be destroyed before we've finished working with it.

  m_prev->m_next.swap(m_next);
  assert(m_next.get() == this);

  m_prev->m_next->m_prev = m_prev;
  m_prev = nullptr;

  // This may or may not destroy this object,
  // depending on if there are other references to it.
  m_next.reset();
}

bool SplineVertex::hasAtLeastSiblings(const int num) {
  int todo = num;
  for (SplineVertex::Ptr node(this); (node = node->next(LOOP)).get() != this;) {
    if (--todo == 0) {
      return true;
    }
  }

  return false;
}

SplineVertex::Ptr SplineVertex::prev(const Loop loop) {
  return m_prev->thisOrPrevReal(loop);
}

SplineVertex::Ptr SplineVertex::next(const Loop loop) {
  return m_next->thisOrNextReal(loop);
}

SplineVertex::Ptr SplineVertex::insertBefore(const QPointF& pt) {
  auto new_vertex = make_intrusive<RealSplineVertex>(pt, m_prev, this);
  m_prev->m_next = new_vertex;
  m_prev = new_vertex.get();

  return new_vertex;
}

SplineVertex::Ptr SplineVertex::insertAfter(const QPointF& pt) {
  auto new_vertex = make_intrusive<RealSplineVertex>(pt, this, m_next.get());
  m_next->m_prev = new_vertex.get();
  m_next = new_vertex;

  return new_vertex;
}

/*========================= SentinelSplineVertex =======================*/

SentinelSplineVertex::SentinelSplineVertex() : SplineVertex(this, this), m_bridged(false) {}

SentinelSplineVertex::~SentinelSplineVertex() {
  // Just releasing m_next is not enough, because in case some external
  // object holds a reference to a vertix of this spline, that vertex will
  // still (possibly indirectly) reference us through a chain of m_next
  // smart pointers.  Therefore, we explicitly unlink each node.
  while (m_next.get() != this) {
    m_next->remove();
  }
}

SplineVertex::Ptr SentinelSplineVertex::thisOrPrevReal(const Loop loop) {
  if ((loop == LOOP) || ((loop == LOOP_IF_BRIDGED) && m_bridged)) {
    return SplineVertex::Ptr(m_prev);
  } else {
    return nullptr;
  }
}

SplineVertex::Ptr SentinelSplineVertex::thisOrNextReal(const Loop loop) {
  if ((loop == LOOP) || ((loop == LOOP_IF_BRIDGED) && m_bridged)) {
    return m_next;
  } else {
    return nullptr;
  }
}

const QPointF SentinelSplineVertex::point() const {
  assert(!"Illegal call to SentinelSplineVertex::point()");

  return QPointF();
}

void SentinelSplineVertex::setPoint(const QPointF& pt) {
  assert(!"Illegal call to SentinelSplineVertex::setPoint()");
}

void SentinelSplineVertex::remove() {
  assert(!"Illegal call to SentinelSplineVertex::remove()");
}

SplineVertex::Ptr SentinelSplineVertex::firstVertex() const {
  if (m_next.get() == this) {
    return nullptr;
  } else {
    return m_next;
  }
}

SplineVertex::Ptr SentinelSplineVertex::lastVertex() const {
  if (m_prev == this) {
    return nullptr;
  } else {
    return SplineVertex::Ptr(m_prev);
  }
}

/*============================== RealSplineVertex ============================*/

RealSplineVertex::RealSplineVertex(const QPointF& pt, SplineVertex* prev, SplineVertex* next)
    : SplineVertex(prev, next), m_point(pt), m_counter(0) {}

void RealSplineVertex::ref() const {
  ++m_counter;
}

void RealSplineVertex::unref() const {
  if (--m_counter == 0) {
    delete this;
  }
}

SplineVertex::Ptr RealSplineVertex::thisOrPrevReal(Loop) {
  return SplineVertex::Ptr(this);
}

SplineVertex::Ptr RealSplineVertex::thisOrNextReal(Loop loop) {
  return SplineVertex::Ptr(this);
}

const QPointF RealSplineVertex::point() const {
  return m_point;
}

void RealSplineVertex::setPoint(const QPointF& pt) {
  m_point = pt;
}
