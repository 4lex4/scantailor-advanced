/*
   Scan Tailor - Interactive post-processing tool for scanned pages.
   Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef INTERACTIVE_XSPLINE_H_
#define INTERACTIVE_XSPLINE_H_

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
  typedef boost::function<QPointF(const QPointF&)> Transform;
  typedef boost::function<void()> ModifiedCallback;
  typedef boost::function<void()> DragFinishedCallback;

  InteractiveXSpline();

  void setSpline(const XSpline& spline);

  const XSpline& spline() const { return m_spline; }

  void setStorageTransform(const Transform& from_storage, const Transform& to_storage);

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
  void onProximityUpdate(const QPointF& screen_mouse_pos, InteractionState& interaction) override;

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


#endif  // ifndef INTERACTIVE_XSPLINE_H_
