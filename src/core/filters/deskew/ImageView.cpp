// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageView.h"

#include <Constants.h>
#include <core/IconProvider.h>

#include <QAction>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QWheelEvent>
#include <boost/bind.hpp>

#include "ImagePresentation.h"

namespace deskew {
const double ImageView::m_maxRotationDeg = 45.0;
const double ImageView::m_maxRotationSin = std::sin(m_maxRotationDeg * constants::DEG2RAD);
const int ImageView::m_cellSize = 20;

ImageView::ImageView(const QImage& image, const QImage& downscaledImage, const ImageTransformation& xform)
    : ImageViewBase(image, downscaledImage, ImagePresentation(xform.transform(), xform.resultingPreCropArea())),
      m_handlePixmap(IconProvider::getInstance().getIcon("aqua-sphere").pixmap(16, 16)),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_xform(xform) {
  setMouseTracking(true);

  interactionState().setDefaultStatusTip(tr("Use Ctrl+Wheel to rotate or Ctrl+Shift+Wheel for finer rotation."));

  const QString tip(tr("Drag this handle to rotate the image."));
  const double hitRadius = std::max<double>(0.5 * m_handlePixmap.width(), 15.0);
  for (int i = 0; i < 2; ++i) {
    m_handles[i].setHitRadius(hitRadius);
    m_handles[i].setPositionCallback(boost::bind(&ImageView::handlePosition, this, i));
    m_handles[i].setMoveRequestCallback(boost::bind(&ImageView::handleMoveRequest, this, i, _1));
    m_handles[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));

    m_handleInteractors[i].setProximityStatusTip(tip);
    m_handleInteractors[i].setObject(&m_handles[i]);

    makeLastFollower(m_handleInteractors[i]);
  }

  m_zoomHandler.setFocus(ZoomHandler::CENTER);

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  auto* rotateLeft = new QAction(nullptr);
  rotateLeft->setShortcut(QKeySequence(","));
  connect(rotateLeft, SIGNAL(triggered(bool)), SLOT(doRotateLeft()));
  addAction(rotateLeft);

  auto* rotateRight = new QAction(nullptr);
  rotateRight->setShortcut(QKeySequence("."));
  connect(rotateRight, SIGNAL(triggered(bool)), SLOT(doRotateRight()));
  addAction(rotateRight);
}

ImageView::~ImageView() = default;

void ImageView::doRotate(double deg) {
  manualDeskewAngleSetExternally(m_xform.postRotation() + deg);
  emit manualDeskewAngleSet(m_xform.postRotation());
}

void ImageView::doRotateLeft() {
  doRotate(-0.10);
}

void ImageView::doRotateRight() {
  doRotate(0.10);
}

void ImageView::manualDeskewAngleSetExternally(const double degrees) {
  if (m_xform.postRotation() == degrees) {
    return;
  }

  m_xform.setPostRotation(degrees);
  updateTransform(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}

void ImageView::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHints(QPainter::Antialiasing, false);

  const double w = maxViewportRect().width();
  const double h = maxViewportRect().height();
  const QPointF center(getImageRotationOrigin());

  // Draw the semi-transparent grid.
  QPen pen(QColor(0, 0, 0xd1, 90));
  pen.setCosmetic(true);
  pen.setWidth(1);
  painter.setPen(pen);
  QVector<QLineF> lines;
  for (double y = center.y(); (y -= m_cellSize) > 0.0;) {
    lines.push_back(QLineF(0.5, y, w - 0.5, y));
  }
  for (double y = center.y(); (y += m_cellSize) < h;) {
    lines.push_back(QLineF(0.5, y, w - 0.5, y));
  }
  for (double x = center.x(); (x -= m_cellSize) > 0.0;) {
    lines.push_back(QLineF(x, 0.5, x, h - 0.5));
  }
  for (double x = center.x(); (x += m_cellSize) < w;) {
    lines.push_back(QLineF(x, 0.5, x, h - 0.5));
  }
  painter.drawLines(lines);

  // Draw the horizontal and vertical line crossing at the center.
  pen.setColor(QColor(0, 0, 0xd1));
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  painter.drawLine(QPointF(0.5, center.y()), QPointF(w - 0.5, center.y()));
  painter.drawLine(QPointF(center.x(), 0.5), QPointF(center.x(), h - 0.5));
  // Draw the rotation arcs.
  // Those will look like this (  )
  const QRectF arcSquare(getRotationArcSquare());

  painter.setRenderHints(QPainter::Antialiasing, true);
  pen.setWidthF(1.5);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  painter.drawArc(arcSquare, qRound(16 * -m_maxRotationDeg), qRound(16 * 2 * m_maxRotationDeg));
  painter.drawArc(arcSquare, qRound(16 * (180 - m_maxRotationDeg)), qRound(16 * 2 * m_maxRotationDeg));

  const std::pair<QPointF, QPointF> handles(getRotationHandles(arcSquare));

  QRectF rect(m_handlePixmap.rect());
  rect.moveCenter(handles.first);
  painter.drawPixmap(rect.topLeft(), m_handlePixmap);
  rect.moveCenter(handles.second);
  painter.drawPixmap(rect.topLeft(), m_handlePixmap);
}  // ImageView::onPaint

void ImageView::onWheelEvent(QWheelEvent* event, InteractionState& interaction) {
  if (interaction.captured()) {
    return;
  }

  double degreeFraction = 0;

  if (event->modifiers() == Qt::ControlModifier) {
    degreeFraction = 0.1;
  } else if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
    degreeFraction = 0.05;
  } else {
    return;
  }

  event->accept();
  const double delta = degreeFraction * event->delta() / 120;
  double angleDeg = m_xform.postRotation() - delta;
  angleDeg = qBound(-m_maxRotationDeg, angleDeg, m_maxRotationDeg);
  if (angleDeg == m_xform.postRotation()) {
    return;
  }

  m_xform.setPostRotation(angleDeg);
  updateTransformPreservingScale(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
  emit manualDeskewAngleSet(m_xform.postRotation());
}  // ImageView::onWheelEvent

QPointF ImageView::handlePosition(int idx) const {
  const std::pair<QPointF, QPointF> handles(getRotationHandles(getRotationArcSquare()));
  if (idx == 0) {
    return handles.first;
  } else {
    return handles.second;
  }
}

void ImageView::handleMoveRequest(int idx, const QPointF& pos) {
  const QRectF arcSquare(getRotationArcSquare());
  const double arcRadius = 0.5 * arcSquare.width();
  const double absY = pos.y();
  double relY = absY - arcSquare.center().y();
  relY = qBound(-arcRadius, relY, arcRadius);

  double angleRad = std::asin(relY / arcRadius);
  if (idx == 0) {
    angleRad = -angleRad;
  }
  double angleDeg = angleRad * constants::RAD2DEG;
  angleDeg = qBound(-m_maxRotationDeg, angleDeg, m_maxRotationDeg);
  if (angleDeg == m_xform.postRotation()) {
    return;
  }

  m_xform.setPostRotation(angleDeg);
  updateTransformPreservingScale(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}

void ImageView::dragFinished() {
  emit manualDeskewAngleSet(m_xform.postRotation());
}

/**
 * Get the point at the center of the widget, in widget coordinates.
 * The point may be adjusted to to ensure it's at the center of a pixel.
 */
QPointF ImageView::getImageRotationOrigin() const {
  const QRectF viewportRect(maxViewportRect());
  return QPointF(std::floor(0.5 * viewportRect.width()) + 0.5, std::floor(0.5 * viewportRect.height()) + 0.5);
}

/**
 * Get the square in widget coordinates where two rotation arcs will be drawn.
 */
QRectF ImageView::getRotationArcSquare() const {
  const double hMargin
      = 0.5 * m_handlePixmap.width()
        + verticalScrollBar()->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, verticalScrollBar());
  const double vMargin
      = 0.5 * m_handlePixmap.height()
        + horizontalScrollBar()->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, horizontalScrollBar());

  QRectF reducedScreenRect(maxViewportRect());
  reducedScreenRect.adjust(hMargin, vMargin, -hMargin, -vMargin);

  QSizeF arcSize(1.0, m_maxRotationSin);
  arcSize.scale(reducedScreenRect.size(), Qt::KeepAspectRatio);
  arcSize.setHeight(arcSize.width());

  QRectF arcSquare(QPointF(0, 0), arcSize);
  arcSquare.moveCenter(reducedScreenRect.center());
  return arcSquare;
}

std::pair<QPointF, QPointF> ImageView::getRotationHandles(const QRectF& arcSquare) const {
  const double rotSin = m_xform.postRotationSin();
  const double rotCos = m_xform.postRotationCos();
  const double arcRadius = 0.5 * arcSquare.width();
  const QPointF arcCenter(arcSquare.center());
  QPointF leftHandle(-rotCos * arcRadius, -rotSin * arcRadius);
  leftHandle += arcCenter;
  QPointF rightHandle(rotCos * arcRadius, rotSin * arcRadius);
  rightHandle += arcCenter;
  return std::make_pair(leftHandle, rightHandle);
}
}  // namespace deskew