// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_INTERACTIVEXSPLINE_H_
#define SCANTAILOR_INTERACTION_INTERACTIVEXSPLINE_H_

#include <QCoreApplication>
#include <QPointF>
#include <boost/function.hpp>
#include <boost/scoped_array.hpp>
#include <cstddef>
#include "DraggablePoint.h"
#include "InteractionState.h"
#include "ObjectDragHandler.h"
#include "VecNT.h"
#include "XSpline.h"

class InteractiveXSpline : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(InteractiveXSpline)
 public:
  using Transform = boost::function<QPointF(const QPointF&)>;
  using ModifiedCallback = boost::function<void()>;
  using DragFinishedCallback = boost::function<void()>;

  InteractiveXSpline();

  void setSpline(const XSpline& spline);

  const XSpline& spline() const { return m_spline; }

  void setStorageTransform(const Transform& fromStorage, const Transform& toStorage);

  void setModifiedCallback(const ModifiedCallback& callback);

  void setDragFinishedCallback(const DragFinishedCallback& callback);

  /**
   * \brief Returns true if the curve is a proximity leader.
   *
   * \param state Interaction state, used to tell whether
   *              the curve is the proximity leader.
   * \param pt If provided, the point on the curve closest to
   *           the cursor will be written there.
   * \param t If provided, the splie's T parameter corresponding
   *          to the point closest to the cursor will be written there.
   * \return true if the curve is the proximity leader.
   */
  bool curveIsProximityLeader(const InteractionState& state, QPointF* pt = nullptr, double* t = nullptr) const;

 protected:
  void onProximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

 private:
  struct NoOp;
  struct IdentTransform;

  struct ControlPoint {
    DraggablePoint point;
    ObjectDragHandler handler;

    ControlPoint() = default;
  };

  QPointF controlPointPosition(int idx) const;

  void controlPointMoveRequest(int idx, const QPointF& pos, Qt::KeyboardModifiers mask);

  void dragFinished();

  static Vec4d rotationAndScale(const QPointF& from, const QPointF& to);

  ModifiedCallback m_modifiedCallback;
  DragFinishedCallback m_dragFinishedCallback;
  Transform m_fromStorage;
  Transform m_toStorage;
  XSpline m_spline;
  boost::scoped_array<ControlPoint> m_controlPoints;
  InteractionState::Captor m_curveProximity;
  QPointF m_curveProximityPointStorage;
  QPointF m_curveProximityPointScreen;
  double m_curveProximityT;
  bool m_lastProximity;
};


#endif  // ifndef SCANTAILOR_INTERACTION_INTERACTIVEXSPLINE_H_
