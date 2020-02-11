// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_SPLINEVERTEX_H_
#define SCANTAILOR_ZONES_SPLINEVERTEX_H_

#include <QPointF>
#include <memory>

#include "NonCopyable.h"

class SplineVertex : public std::enable_shared_from_this<SplineVertex> {
 public:
  enum Loop { LOOP, NO_LOOP, LOOP_IF_BRIDGED };

  using Ptr = std::shared_ptr<SplineVertex>;

  SplineVertex();

  SplineVertex(SplineVertex* prev, SplineVertex* next);

  virtual ~SplineVertex() = default;

  /**
   * \return Smart pointer to this vertex, unless it's a sentinel vertex,
   *         in which case the previous non-sentinel vertex is returned.
   *         If there are no non-sentinel vertices, a null smart pointer
   *         is returned.
   */
  virtual SplineVertex::Ptr thisOrPrevReal(Loop loop) = 0;

  /**
   * \return Smart pointer to this vertex, unless it's a sentinel vertex,
   *         in which case the next non-sentinel vertex is returned.
   *         If there are no non-sentinel vertices, a null smart pointer
   *         is returned.
   */
  virtual SplineVertex::Ptr thisOrNextReal(Loop loop) = 0;

  virtual const QPointF& point() const = 0;

  virtual void setPoint(const QPointF& pt) = 0;

  virtual void remove();

  bool hasAtLeastSiblings(int num);

  SplineVertex::Ptr prev(Loop loop);

  SplineVertex::Ptr next(Loop loop);

  SplineVertex::Ptr insertBefore(const QPointF& pt);

  SplineVertex::Ptr insertAfter(const QPointF& pt);

 protected:
  /**
   * Usually we have circular dependency of m_next pointers here
   * so we unlink a vertex from the previous one to have
   * that pointer loop broken to be able to destruct this spline.
   */
  void unlinkWithPrevious();

  /**
   * m_prev have to be an ordinary pointer or weak_ptr in order
   * to avoid circular dependencies.
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

  const QPointF& point() const override;

  void setPoint(const QPointF& pt) override;

  void remove() override;

  SplineVertex::Ptr firstVertex() const;

  SplineVertex::Ptr lastVertex() const;

  /**
   * Making this sentinel be ready for operation.
   * This method must usually be called in a constructor
   * of the class that have the ownership of this sentinel.
   */
  void init();

  /**
   * Preparing this sentinel to be destroyed.
   * This method must usually be called in the destructor
   * of the class that have the ownership of this sentinel.
   */
  void finalize();

  bool bridged() const { return m_bridged; }

  void setBridged(bool bridged) { m_bridged = bridged; }

 private:
  bool m_bridged;
};


class RealSplineVertex : public SplineVertex {
  DECLARE_NON_COPYABLE(RealSplineVertex)
 public:
  RealSplineVertex(const QPointF& pt, SplineVertex* prev, SplineVertex* next);

  SplineVertex::Ptr thisOrPrevReal(Loop loop) override;

  SplineVertex::Ptr thisOrNextReal(Loop loop) override;

  const QPointF& point() const override;

  void setPoint(const QPointF& pt) override;

 private:
  QPointF m_point;
};


#endif  // ifndef SCANTAILOR_ZONES_SPLINEVERTEX_H_
