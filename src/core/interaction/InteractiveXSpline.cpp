// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  const int numControlPoints = spline.numControlPoints();

  XSpline newSpline(spline);
  boost::scoped_array<ControlPoint> newControlPoints(new ControlPoint[numControlPoints]);

  for (int i = 0; i < numControlPoints; ++i) {
    newControlPoints[i].point.setPositionCallback(boost::bind(&InteractiveXSpline::controlPointPosition, this, i));
    newControlPoints[i].point.setMoveRequestCallback(
        boost::bind(&InteractiveXSpline::controlPointMoveRequest, this, i, _1, _2));
    newControlPoints[i].point.setDragFinishedCallback(boost::bind(&InteractiveXSpline::dragFinished, this));

    if ((i == 0) || (i == numControlPoints - 1)) {
      newControlPoints[i].handler.setKeyboardModifiers(
          {Qt::NoModifier, Qt::ShiftModifier, Qt::ControlModifier, Qt::ShiftModifier | Qt::ControlModifier});
      // Endpoints can't be deleted.
      newControlPoints[i].handler.setProximityStatusTip(
          tr("This point can be dragged. Hold Ctrl or Shift to drag along axes."));
    } else {
      newControlPoints[i].handler.setProximityStatusTip(tr("Drag this point or delete it by pressing Del or D."));
    }
    newControlPoints[i].handler.setInteractionCursor(Qt::BlankCursor);
    newControlPoints[i].handler.setObject(&newControlPoints[i].point);

    makeLastFollower(newControlPoints[i].handler);
  }

  m_spline.swap(newSpline);
  m_controlPoints.swap(newControlPoints);

  m_modifiedCallback();
}  // InteractiveXSpline::setSpline

void InteractiveXSpline::setStorageTransform(const Transform& fromStorage, const Transform& toStorage) {
  m_fromStorage = fromStorage;
  m_toStorage = toStorage;
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

void InteractiveXSpline::onProximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) {
  m_curveProximityPointStorage = m_spline.pointClosestTo(m_toStorage(screenMousePos), &m_curveProximityT);
  m_curveProximityPointScreen = m_fromStorage(m_curveProximityPointStorage);

  const Proximity proximity(screenMousePos, m_curveProximityPointScreen);
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
    const int pntIdx = segment + 1;

    m_spline.insertControlPoint(pntIdx, m_curveProximityPointStorage, 1);
    setSpline(m_spline);

    m_controlPoints[pntIdx].handler.forceEnterDragState(interaction, event->pos());
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
      const int numControlPoints = m_spline.numControlPoints();
      // Check if one of our control points is a proximity leader.
      // Note that we don't consider the endpoints.
      for (int i = 1; i < numControlPoints - 1; ++i) {
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
  const QPointF storagePt(m_toStorage(pos));
  bool swapSides = false;
  bool modified = mask.testFlag(Qt::ControlModifier) || mask.testFlag(Qt::ShiftModifier);

  if (modified) {
    double ang = findAngle(m_toStorage(QPointF(0, 0)), m_toStorage(QPointF(1, 0)));
    swapSides = (ang >= 45 && ang <= 135);  // page has been rotated

    if (mask.testFlag(Qt::ShiftModifier)) {
      swapSides = !swapSides;
    }
  }

  const int numControlPoints = m_spline.numControlPoints();
  if ((idx > 0) && (idx < numControlPoints - 1)) {
    // A midpoint - just move it.
    m_spline.moveControlPoint(idx, storagePt);
  } else {
    // An endpoint was moved.  Instead of moving it on its own,
    // we are going to rotate and / or scale all of the points
    // relative to the opposite endpoint.
    const int originIdx = idx == 0 ? numControlPoints - 1 : 0;
    const QPointF origin(m_spline.controlPointPosition(originIdx));
    const QPointF oldPos(m_spline.controlPointPosition(idx));
    if (Vec2d(oldPos - origin).squaredNorm() > 1.0) {
      // rotationAndScale() would throw an exception if oldPos == origin.
      const Vec4d mat(rotationAndScale(oldPos - origin, storagePt - origin));
      for (int i = 0; i < numControlPoints; ++i) {
        Vec2d pt(m_spline.controlPointPosition(i) - origin);
        MatrixCalc<double> mc;
        (mc(mat, 2, 2) * mc(pt, 2, 1)).write(pt);

        if (!modified) {  // default behavior
          m_spline.moveControlPoint(i, pt + origin);
        } else {  // Ctrl or Shift is pressed
          Vec2d shift = storagePt - oldPos;
          QPointF newPosition = m_spline.controlPointPosition(i) + shift;

          if (!swapSides) {
            newPosition.setX(m_spline.controlPointPosition(i).x());
          } else {
            newPosition.setY(m_spline.controlPointPosition(i).y());
          }

          m_spline.moveControlPoint(i, newPosition);
        }
      }
    } else {
      // Move the endpoint and distribute midpoints uniformly.
      const QLineF line(origin, storagePt);
      const double scale = 1.0 / (numControlPoints - 1);
      for (int i = 0; i < numControlPoints; ++i) {
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
