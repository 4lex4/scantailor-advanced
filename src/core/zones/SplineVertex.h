// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
   * The reason m_prev is an ordinary pointer rather than a smart pointer
   * is that we don't want pairs of vertices holding smart pointers to each
   * other.  Note that we don't have a loop of smart pointers, because
   * sentinel vertices aren't reference counted.
   */
  SplineVertex* m_prev;
  SplineVertex::Ptr m_next;
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
  mutable int m_counter;
};


#endif  // ifndef SPLINE_VERTEX_H_
