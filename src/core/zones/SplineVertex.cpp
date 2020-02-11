// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SplineVertex.h"

#include <cassert>


/*============================= SplineVertex ============================*/

SplineVertex::SplineVertex() : m_prev(nullptr), m_next(nullptr) {}

SplineVertex::SplineVertex(SplineVertex* prev, SplineVertex* next) : m_prev(prev), m_next(next->shared_from_this()) {}

void SplineVertex::remove() {
  // Be very careful here - don't let this object
  // be destroyed before we've finished working with it.
  if (m_next)
    m_next->m_prev = m_prev;
  if (m_prev)
    m_prev->m_next.swap(m_next);
  assert(!m_next || (m_next.get() == this));

  m_prev = nullptr;
  // This may or may not destroy this object,
  // depending on if there are other references to it.
  m_next.reset();
}

bool SplineVertex::hasAtLeastSiblings(const int num) {
  int todo = num;
  for (SplineVertex::Ptr node = shared_from_this(); (node = node->next(LOOP)).get() != this;) {
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
  auto newVertex = std::make_shared<RealSplineVertex>(pt, m_prev, this);
  m_prev->m_next = newVertex;
  m_prev = newVertex.get();
  return newVertex;
}

SplineVertex::Ptr SplineVertex::insertAfter(const QPointF& pt) {
  auto newVertex = std::make_shared<RealSplineVertex>(pt, this, m_next.get());
  m_next->m_prev = newVertex.get();
  m_next = newVertex;
  return newVertex;
}

void SplineVertex::unlinkWithPrevious() {
  m_prev->m_next.reset();
  m_prev = nullptr;
}

/*========================= SentinelSplineVertex =======================*/

SentinelSplineVertex::SentinelSplineVertex() : m_bridged(false) {}

SentinelSplineVertex::~SentinelSplineVertex() {
  // Just releasing m_next is not enough, because in case some external
  // object holds a reference to a vertex of this spline, that vertex will
  // still (possibly indirectly) reference us through a chain of m_next
  // smart pointers. Therefore, we explicitly unlink each node.
  while (m_next) {
    m_next->remove();
  }
}

SplineVertex::Ptr SentinelSplineVertex::thisOrPrevReal(const Loop loop) {
  if ((loop == LOOP) || ((loop == LOOP_IF_BRIDGED) && m_bridged)) {
    return m_prev->shared_from_this();
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

const QPointF& SentinelSplineVertex::point() const {
  throw std::logic_error("Illegal call to SentinelSplineVertex::point()");
}

void SentinelSplineVertex::setPoint(const QPointF&) {
  throw std::logic_error("Illegal call to SentinelSplineVertex::setPoint()");
}

void SentinelSplineVertex::remove() {
  throw std::logic_error("Illegal call to SentinelSplineVertex::remove()");
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
    return m_prev->shared_from_this();
  }
}

void SentinelSplineVertex::finalize() {
  SplineVertex::unlinkWithPrevious();
}

void SentinelSplineVertex::init() {
  if (!m_next && !m_prev) {
    m_next = shared_from_this();
    m_prev = this;
  }
}

/*============================== RealSplineVertex ============================*/

RealSplineVertex::RealSplineVertex(const QPointF& pt, SplineVertex* prev, SplineVertex* next)
    : SplineVertex(prev, next), m_point(pt) {}

SplineVertex::Ptr RealSplineVertex::thisOrPrevReal(Loop) {
  return shared_from_this();
}

SplineVertex::Ptr RealSplineVertex::thisOrNextReal(Loop) {
  return shared_from_this();
}

const QPointF& RealSplineVertex::point() const {
  return m_point;
}

void RealSplineVertex::setPoint(const QPointF& pt) {
  m_point = pt;
}
