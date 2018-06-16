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

#ifndef SPLINE_VERTEX_H_
#define SPLINE_VERTEX_H_

#include <QPointF>
#include "NonCopyable.h"
#include "intrusive_ptr.h"

class SplineVertex {
 public:
  enum Loop { LOOP, NO_LOOP, LOOP_IF_BRIDGED };

  typedef intrusive_ptr<SplineVertex> Ptr;

  SplineVertex(SplineVertex* prev, SplineVertex* next);

  virtual ~SplineVertex() = default;

  /**
   * We don't want reference counting for sentinel vertices,
   * but we can't make ref() and unref() abstract here, because
   * in case of sentinel vertices these function may actually
   * be called from this class constructor.
   */
  virtual void ref() const {}

  /**
   * \see ref()
   */
  virtual void unref() const {}

  /**
   * \return Smart pointer to this vertex, unless it's a sentiel vertex,
   *         in which case the previous non-sentinel vertex is returned.
   *         If there are no non-sentinel vertices, a null smart pointer
   *         is returned.
   */
  virtual SplineVertex::Ptr thisOrPrevReal(Loop loop) = 0;

  /**
   * \return Smart pointer to this vertex, unless it's a sentiel vertex,
   *         in which case the next non-sentinel vertex is returned.
   *         If there are no non-sentinel vertices, a null smart pointer
   *         is returned.
   */
  virtual SplineVertex::Ptr thisOrNextReal(Loop loop) = 0;

  virtual const QPointF point() const = 0;

  virtual void setPoint(const QPointF& pt) = 0;

  virtual void remove();

  bool hasAtLeastSiblings(int num);

  SplineVertex::Ptr prev(Loop loop);

  SplineVertex::Ptr next(Loop loop);

  SplineVertex::Ptr insertBefore(const QPointF& pt);

  SplineVertex::Ptr insertAfter(const QPointF& pt);

 protected:
  /**
   * The reason m_pPrev is an ordinary pointer rather than a smart pointer
   * is that we don't want pairs of vertices holding smart pointers to each
   * other.  Note that we don't have a loop of smart pointers, because
   * sentinel vertices aren't reference counted.
   */
  SplineVertex* m_pPrev;
  SplineVertex::Ptr m_ptrNext;
};


class SentinelSplineVertex : public SplineVertex {
  DECLARE_NON_COPYABLE(SentinelSplineVertex)

 public:
  SentinelSplineVertex();

  ~SentinelSplineVertex() override;

  SplineVertex::Ptr thisOrPrevReal(Loop loop) override;

  SplineVertex::Ptr thisOrNextReal(Loop loop) override;

  const QPointF point() const override;

  void setPoint(const QPointF& pt) override;

  void remove() override;

  SplineVertex::Ptr firstVertex() const;

  SplineVertex::Ptr lastVertex() const;

  bool bridged() const { return m_bridged; }

  void setBridged(bool bridged) { m_bridged = bridged; }

 private:
  bool m_bridged;
};


class RealSplineVertex : public SplineVertex {
  DECLARE_NON_COPYABLE(RealSplineVertex)

 public:
  RealSplineVertex(const QPointF& pt, SplineVertex* prev, SplineVertex* next);

  void ref() const override;

  void unref() const override;

  SplineVertex::Ptr thisOrPrevReal(Loop loop) override;

  SplineVertex::Ptr thisOrNextReal(Loop loop) override;

  const QPointF point() const override;

  void setPoint(const QPointF& pt) override;

 private:
  QPointF m_point;
  mutable int m_refCounter;
};


#endif  // ifndef SPLINE_VERTEX_H_
