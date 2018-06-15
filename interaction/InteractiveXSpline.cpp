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

#include "InteractiveXSpline.h"
#include <QCursor>
#include <QDebug>
#include <QMouseEvent>
#include <Qt>
#include "MatrixCalc.h"
#include "Proximity.h"
#include "VecNT.h"

#ifndef Q_MOC_RUN

#include <boost/bind.hpp>

#endif

struct InteractiveXSpline::NoOp {
  void operator()() const {}
};

struct InteractiveXSpline::IdentTransform {
  QPointF operator()(const QPointF& pt) const { return pt; }
};

InteractiveXSpline::InteractiveXSpline()
    : m_modifiedCallback(NoOp()),
      m_dragFinishedCallback(NoOp()),
      m_fromStorage(IdentTransform()),
      m_toStorage(IdentTransform()),
      m_curveProximityT(),
      m_lastProximity(false) {
  m_curveProximity.setProximityCursor(Qt::PointingHandCursor);
  m_curveProximity.setProximityStatusTip(tr("Click to create a new control point."));
}

void InteractiveXSpline::setSpline(const XSpline& spline) {
  const int num_control_points = spline.numControlPoints();

  XSpline new_spline(spline);
  boost::scoped_array<ControlPoint> new_control_points(new ControlPoint[num_control_points]);

  for (int i = 0; i < num_control_points; ++i) {
    new_control_points[i].point.setPositionCallback(boost::bind(&InteractiveXSpline::controlPointPosition, this, i));
    new_control_points[i].point.setMoveRequestCallback(
        boost::bind(&InteractiveXSpline::controlPointMoveRequest, this, i, _1, _2));
    new_control_points[i].point.setDragFinishedCallback(boost::bind(&InteractiveXSpline::dragFinished, this));

    if ((i == 0) || (i == num_control_points - 1)) {
      new_control_points[i].handler.setKeyboardModifiers(
          {Qt::NoModifier, Qt::ShiftModifier, Qt::ControlModifier, Qt::ShiftModifier | Qt::ControlModifier});
      // Endpoints can't be deleted.
      new_control_points[i].handler.setProximityStatusTip(
          tr("This point can be dragged. Hold Ctrl or Shift to drag along axes."));
    } else {
      new_control_points[i].handler.setProximityStatusTip(tr("Drag this point or delete it by pressing Del or D."));
    }
    new_control_points[i].handler.setInteractionCursor(Qt::BlankCursor);
    new_control_points[i].handler.setObject(&new_control_points[i].point);

    makeLastFollower(new_control_points[i].handler);
  }

  m_spline.swap(new_spline);
  m_controlPoints.swap(new_control_points);

  m_modifiedCallback();
}  // InteractiveXSpline::setSpline

void InteractiveXSpline::setStorageTransform(const Transform& from_storage, const Transform& to_storage) {
  m_fromStorage = from_storage;
  m_toStorage = to_storage;
}

void InteractiveXSpline::setModifiedCallback(const ModifiedCallback& callback) {
  m_modifiedCallback = callback;
}

void InteractiveXSpline::setDragFinishedCallback(const DragFinishedCallback& callback) {
  m_dragFinishedCallback = callback;
}

bool InteractiveXSpline::curveIsProximityLeader(const InteractionState& state, QPointF* pt, double* t) const {
  if (state.proximityLeader(m_curveProximity)) {
    if (pt) {
      *pt = m_curveProximityPointScreen;
    }
    if (t) {
      *t = m_curveProximityT;
    }
    return true;
  }

  return false;
}

void InteractiveXSpline::onProximityUpdate(const QPointF& screen_mouse_pos, InteractionState& interaction) {
  m_curveProximityPointStorage = m_spline.pointClosestTo(m_toStorage(screen_mouse_pos), &m_curveProximityT);
  m_curveProximityPointScreen = m_fromStorage(m_curveProximityPointStorage);

  const Proximity proximity(screen_mouse_pos, m_curveProximityPointScreen);
  interaction.updateProximity(m_curveProximity, proximity, -1);
}

void InteractiveXSpline::onMouseMoveEvent(QMouseEvent*, InteractionState& interaction) {
  if (interaction.proximityLeader(m_curveProximity)) {
    // We need to redraw the highlighted point.
    interaction.setRedrawRequested(true);
    m_lastProximity = true;
  } else if (m_lastProximity) {
    // In this case we need to un-draw the highlighted point.
    interaction.setRedrawRequested(true);
    m_lastProximity = false;
  }
}

void InteractiveXSpline::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  if (interaction.captured()) {
    return;
  }

  if (interaction.proximityLeader(m_curveProximity)) {
    const auto segment = int(m_curveProximityT * m_spline.numSegments());
    const int pnt_idx = segment + 1;

    m_spline.insertControlPoint(pnt_idx, m_curveProximityPointStorage, 1);
    setSpline(m_spline);

    m_controlPoints[pnt_idx].handler.forceEnterDragState(interaction, event->pos());
    event->accept();

    interaction.setRedrawRequested(true);
  }
}

void InteractiveXSpline::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  if (interaction.captured()) {
    return;
  }

  switch (event->key()) {
    case Qt::Key_Delete:
    case Qt::Key_D:
      const int num_control_points = m_spline.numControlPoints();
      // Check if one of our control points is a proximity leader.
      // Note that we don't consider the endpoints.
      for (int i = 1; i < num_control_points - 1; ++i) {
        if (m_controlPoints[i].handler.proximityLeader(interaction)) {
          m_spline.eraseControlPoint(i);
          setSpline(m_spline);
          interaction.setRedrawRequested(true);
          event->accept();
          break;
        }
      }
  }
}

QPointF InteractiveXSpline::controlPointPosition(int idx) const {
  return m_fromStorage(m_spline.controlPointPosition(idx));
}

double findAngle(QPointF p1, QPointF p2) {  // return angle between vector (0,0)(1,0) and this
  // move p1 to 0,0
  p2 -= p1;

  // get angle with (1,0)
  double a = p2.x() / std::sqrt(p2.x() * p2.x() + p2.y() * p2.y());
  return std::acos(a) * 180.0 / 3.14159265;
}

void InteractiveXSpline::controlPointMoveRequest(int idx, const QPointF& pos, Qt::KeyboardModifiers mask) {
  const QPointF storage_pt(m_toStorage(pos));
  bool swap_sides = false;
  bool modified = mask.testFlag(Qt::ControlModifier) || mask.testFlag(Qt::ShiftModifier);

  if (modified) {
    double ang = findAngle(m_toStorage(QPointF(0, 0)), m_toStorage(QPointF(1, 0)));
    swap_sides = (ang >= 45 && ang <= 135);  // page has been rotated

    if (mask.testFlag(Qt::ShiftModifier)) {
      swap_sides = !swap_sides;
    }
  }

  const int num_control_points = m_spline.numControlPoints();
  if ((idx > 0) && (idx < num_control_points - 1)) {
    // A midpoint - just move it.
    m_spline.moveControlPoint(idx, storage_pt);
  } else {
    // An endpoint was moved.  Instead of moving it on its own,
    // we are going to rotate and / or scale all of the points
    // relative to the opposite endpoint.
    const int origin_idx = idx == 0 ? num_control_points - 1 : 0;
    const QPointF origin(m_spline.controlPointPosition(origin_idx));
    const QPointF old_pos(m_spline.controlPointPosition(idx));
    if (Vec2d(old_pos - origin).squaredNorm() > 1.0) {
      // rotationAndScale() would throw an exception if old_pos == origin.
      const Vec4d mat(rotationAndScale(old_pos - origin, storage_pt - origin));
      for (int i = 0; i < num_control_points; ++i) {
        Vec2d pt(m_spline.controlPointPosition(i) - origin);
        MatrixCalc<double> mc;
        (mc(mat, 2, 2) * mc(pt, 2, 1)).write(pt);

        if (!modified) {  // default behavior
          m_spline.moveControlPoint(i, pt + origin);
        } else {  // Ctrl or Shift is pressed
          Vec2d shift = storage_pt - old_pos;
          QPointF new_position = m_spline.controlPointPosition(i) + shift;

          if (!swap_sides) {
            new_position.setX(m_spline.controlPointPosition(i).x());
          } else {
            new_position.setY(m_spline.controlPointPosition(i).y());
          }

          m_spline.moveControlPoint(i, new_position);
        }
      }
    } else {
      // Move the endpoint and distribute midpoints uniformly.
      const QLineF line(origin, storage_pt);
      const double scale = 1.0 / (num_control_points - 1);
      for (int i = 0; i < num_control_points; ++i) {
        m_spline.moveControlPoint(i, line.pointAt(i * scale));
      }
    }
  }

  m_modifiedCallback();
}  // InteractiveXSpline::controlPointMoveRequest

void InteractiveXSpline::dragFinished() {
  m_dragFinishedCallback();
}

Vec4d InteractiveXSpline::rotationAndScale(const QPointF& from, const QPointF& to) {
  Vec4d A;
  A[0] = from.x();
  A[1] = from.y();
  A[2] = from.y();
  A[3] = -from.x();

  Vec2d B(to.x(), to.y());

  Vec2d x;
  MatrixCalc<double> mc;
  mc(A, 2, 2).solve(mc(B, 2, 1)).write(x);

  A[0] = x[0];
  A[1] = -x[1];
  A[2] = x[1];
  A[3] = x[0];
  return A;
}
