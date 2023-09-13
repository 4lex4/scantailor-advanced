// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageView.h"

#include <core/IconProvider.h>

#include <QDebug>
#include <QPainter>
#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <utility>

#include "ImagePresentation.h"
#include "ImageTransformation.h"
#include "ProjectPages.h"

namespace page_split {
ImageView::ImageView(const QImage& image,
                     const QImage& downscaledImage,
                     const ImageTransformation& xform,
                     const PageLayout& layout,
                     std::shared_ptr<ProjectPages> pages,
                     const ImageId& imageId,
                     bool leftHalfRemoved,
                     bool rightHalfRemoved)
    : ImageViewBase(image, downscaledImage, ImagePresentation(xform.transform(), xform.resultingPreCropArea())),
      m_pages(std::move(pages)),
      m_imageId(imageId),
      m_leftUnremoveButton(boost::bind(&ImageView::leftPageCenter, this)),
      m_rightUnremoveButton(boost::bind(&ImageView::rightPageCenter, this)),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_handlePixmap(IconProvider::getInstance().getIcon("aqua-sphere").pixmap(16, 16)),
      m_virtLayout(layout),
      m_leftPageRemoved(leftHalfRemoved),
      m_rightPageRemoved(rightHalfRemoved) {
  setMouseTracking(true);

  m_leftUnremoveButton.setClickCallback(boost::bind(&ImageView::unremoveLeftPage, this));
  m_rightUnremoveButton.setClickCallback(boost::bind(&ImageView::unremoveRightPage, this));

  if (m_leftPageRemoved) {
    makeLastFollower(m_leftUnremoveButton);
  }
  if (m_rightPageRemoved) {
    makeLastFollower(m_rightUnremoveButton);
  }

  setupCuttersInteraction();

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView() = default;

void ImageView::setupCuttersInteraction() {
  const QString tip(tr("Drag the line or the handles."));
  const double hitRadius = std::max<double>(0.5 * m_handlePixmap.width(), 15.0);
  const int numCutters = m_virtLayout.numCutters();
  for (int i = 0; i < numCutters; ++i) {  // Loop over lines.
    m_lineInteractors[i].setObject(&m_lineSegments[0]);

    for (int j = 0; j < 2; ++j) {  // Loop over handles.
      m_handles[i][j].setHitRadius(hitRadius);
      m_handles[i][j].setPositionCallback(boost::bind(&ImageView::handlePosition, this, i, j));
      m_handles[i][j].setMoveRequestCallback(
          boost::bind(&ImageView::handleMoveRequest, this, i, j, boost::placeholders::_1));
      m_handles[i][j].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));

      m_handleInteractors[i][j].setObject(&m_handles[i][j]);
      m_handleInteractors[i][j].setProximityStatusTip(tip);
      makeLastFollower(m_handleInteractors[i][j]);
    }

    m_lineSegments[i].setPositionCallback(boost::bind(&ImageView::linePosition, this, i));
    m_lineSegments[i].setMoveRequestCallback(
        boost::bind(&ImageView::lineMoveRequest, this, i, boost::placeholders::_1));
    m_lineSegments[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));

    m_lineInteractors[i].setObject(&m_lineSegments[i]);
    m_lineInteractors[i].setProximityCursor(Qt::SplitHCursor);
    m_lineInteractors[i].setInteractionCursor(Qt::SplitHCursor);
    m_lineInteractors[i].setProximityStatusTip(tip);
    makeLastFollower(m_lineInteractors[i]);
  }

  // Turn off cutters we don't need anymore.
  for (int i = numCutters; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      m_handleInteractors[i][j].unlink();
    }
    m_lineInteractors[i].unlink();
  }
}  // ImageView::setupCuttersInteraction

void ImageView::pageLayoutSetExternally(const PageLayout& layout) {
  m_virtLayout = layout;
  setupCuttersInteraction();
  update();
}

void ImageView::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

  painter.setPen(Qt::NoPen);
  const QRectF virtRect(virtualDisplayRect());

  switch (m_virtLayout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      painter.setBrush(QColor(0, 0, 255, 50));
      painter.drawRect(virtRect);
      return;  // No Split Line will be drawn.
    case PageLayout::SINGLE_PAGE_CUT:
      painter.setBrush(QColor(0, 0, 255, 50));
      painter.drawPolygon(m_virtLayout.singlePageOutline());
      break;
    case PageLayout::TWO_PAGES:
      painter.setBrush(m_leftPageRemoved ? QColor(0, 0, 0, 80) : QColor(0, 0, 255, 50));
      painter.drawPolygon(m_virtLayout.leftPageOutline());
      painter.setBrush(m_rightPageRemoved ? QColor(0, 0, 0, 80) : QColor(255, 0, 0, 50));
      painter.drawPolygon(m_virtLayout.rightPageOutline());
      break;
  }

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setWorldTransform(QTransform());

  QPen pen(QColor(0, 0, 255));
  pen.setCosmetic(true);
  pen.setWidth(2);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);

  const int numCutters = m_virtLayout.numCutters();
  for (int i = 0; i < numCutters; ++i) {
    const QLineF cutter(widgetCutterLine(i));
    painter.drawLine(cutter);

    QRectF rect(m_handlePixmap.rect());

    rect.moveCenter(cutter.p1());
    painter.drawPixmap(rect.topLeft(), m_handlePixmap);

    rect.moveCenter(cutter.p2());
    painter.drawPixmap(rect.topLeft(), m_handlePixmap);
  }
}  // ImageView::onPaint

PageLayout ImageView::widgetLayout() const {
  return m_virtLayout.transformed(virtualToWidget());
}

QLineF ImageView::widgetCutterLine(const int lineIdx) const {
  const QRectF widgetRect(virtualToWidget().mapRect(virtualDisplayRect()));
  QRectF reducedWidgetRect(reducedWidgetArea());
  reducedWidgetRect.setLeft(widgetRect.left());
  reducedWidgetRect.setRight(widgetRect.right());
  // The reason we restore original left and right boundaries is that
  // we want to allow cutter handles to go off-screen horizontally
  // but not vertically.
  QLineF line(customInscribedCutterLine(widgetLayout().cutterLine(lineIdx), reducedWidgetRect));

  if (m_handleInteractors[lineIdx][1].interactionInProgress(interactionState())) {
    line.setP1(virtualToWidget().map(virtualCutterLine(lineIdx).p1()));
  } else if (m_handleInteractors[lineIdx][0].interactionInProgress(interactionState())) {
    line.setP2(virtualToWidget().map(virtualCutterLine(lineIdx).p2()));
  }
  return line;
}

QLineF ImageView::virtualCutterLine(int lineIdx) const {
  const QRectF virtRect(virtualDisplayRect());

  QRectF widgetRect(virtualToWidget().mapRect(virtRect));
  const double delta = 0.5 * m_handlePixmap.width();
  widgetRect.adjust(0.0, delta, 0.0, -delta);

  QRectF reducedVirtRect(widgetToVirtual().mapRect(widgetRect));
  reducedVirtRect.setLeft(virtRect.left());
  reducedVirtRect.setRight(virtRect.right());
  // The reason we restore original left and right boundaries is that
  // we want to allow cutter handles to go off-screen horizontally
  // but not vertically.
  return customInscribedCutterLine(m_virtLayout.cutterLine(lineIdx), reducedVirtRect);
}

QRectF ImageView::reducedWidgetArea() const {
  const qreal delta = 0.5 * m_handlePixmap.width();
  return getOccupiedWidgetRect().adjusted(0.0, delta, 0.0, -delta);
}

/**
 * This implementation differs from PageLayout::inscribedCutterLine() in that
 * it forces the endpoints to lie on the top and bottom boundary lines.
 * Line's angle may change as a result.
 */
QLineF ImageView::customInscribedCutterLine(const QLineF& line, const QRectF& rect) {
  if (line.p1().y() == line.p2().y()) {
    // This should not happen, but if it does, we need to handle it gracefully.
    qreal middleX = 0.5 * (line.p1().x() + line.p2().x());
    middleX = qBound(rect.left(), middleX, rect.right());
    return QLineF(QPointF(middleX, rect.top()), QPointF(middleX, rect.bottom()));
  }

  QPointF topPt;
  QPointF bottomPt;

#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
  line.intersect(QLineF(rect.topLeft(), rect.topRight()), &topPt);
  line.intersect(QLineF(rect.bottomLeft(), rect.bottomRight()), &bottomPt);
#else
  line.intersects(QLineF(rect.topLeft(), rect.topRight()), &topPt);
  line.intersects(QLineF(rect.bottomLeft(), rect.bottomRight()), &bottomPt);
#endif

  const double topX = qBound(rect.left(), topPt.x(), rect.right());
  const double bottomX = qBound(rect.left(), bottomPt.x(), rect.right());
  return QLineF(QPointF(topX, rect.top()), QPointF(bottomX, rect.bottom()));
}

QPointF ImageView::handlePosition(int lineIdx, int handleIdx) const {
  const QLineF line(widgetCutterLine(lineIdx));
  return handleIdx == 0 ? line.p1() : line.p2();
}

void ImageView::handleMoveRequest(int lineIdx, int handleIdx, const QPointF& pos) {
  const QRectF validArea(getOccupiedWidgetRect());

  qreal minXTop = validArea.left();
  qreal maxXTop = validArea.right();
  qreal minXBottom = validArea.left();
  qreal maxXBottom = validArea.right();

  // preventing intersection with other cutters
  for (int i = 0; i < m_virtLayout.numCutters(); i++) {
    if (i != lineIdx) {
      QPointF pTopI;
      QPointF pBottomI;
      QLineF anotherLine = virtualToWidget().map(m_virtLayout.cutterLine(i));
#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
      anotherLine.intersect(QLineF(validArea.topLeft(), validArea.topRight()), &pTopI);
      anotherLine.intersect(QLineF(validArea.bottomLeft(), validArea.bottomRight()), &pBottomI);
#else
      anotherLine.intersects(QLineF(validArea.topLeft(), validArea.topRight()), &pTopI);
      anotherLine.intersects(QLineF(validArea.bottomLeft(), validArea.bottomRight()), &pBottomI);
#endif

      if ((pTopI.x() < minXTop) || (pTopI.x() > maxXTop) || (pBottomI.x() < minXBottom)
          || (pBottomI.x() > maxXBottom)) {
        continue;
      }

      if (lineIdx > i) {
        minXTop = pTopI.x();
        minXBottom = pBottomI.x();
      } else {
        maxXTop = pTopI.x();
        maxXBottom = pBottomI.x();
      }
    }
  }

  QLineF virtLine(virtualCutterLine(lineIdx));
  if (handleIdx == 0) {
    const QPointF wpt(qBound(minXTop, pos.x(), maxXTop), validArea.top());
    virtLine.setP1(widgetToVirtual().map(wpt));
  } else {
    const QPointF wpt(qBound(minXBottom, pos.x(), maxXBottom), validArea.bottom());
    virtLine.setP2(widgetToVirtual().map(wpt));
  }

  m_virtLayout.setCutterLine(lineIdx, virtLine);
  update();
}

QLineF ImageView::linePosition(int lineIdx) const {
  return widgetCutterLine(lineIdx);
}

void ImageView::lineMoveRequest(int lineIdx, QLineF line) {
  // Intersect with top and bottom.
  const QRectF validArea(getOccupiedWidgetRect());
  QPointF pTop;
  QPointF pBottom;

#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
  line.intersect(QLineF(validArea.topLeft(), validArea.topRight()), &pTop);
  line.intersect(QLineF(validArea.bottomLeft(), validArea.bottomRight()), &pBottom);
#else
  line.intersects(QLineF(validArea.topLeft(), validArea.topRight()), &pTop);
  line.intersects(QLineF(validArea.bottomLeft(), validArea.bottomRight()), &pBottom);
#endif

  qreal minXTop = validArea.left();
  qreal maxXTop = validArea.right();
  qreal minXBottom = validArea.left();
  qreal maxXBottom = validArea.right();

  // preventing intersection with other cutters
  for (int i = 0; i < m_virtLayout.numCutters(); i++) {
    if (i != lineIdx) {
      QPointF pTopI;
      QPointF pBottomI;
      QLineF anotherLine = virtualToWidget().map(m_virtLayout.cutterLine(i));
#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
      anotherLine.intersect(QLineF(validArea.topLeft(), validArea.topRight()), &pTopI);
      anotherLine.intersect(QLineF(validArea.bottomLeft(), validArea.bottomRight()), &pBottomI);
#else
      anotherLine.intersects(QLineF(validArea.topLeft(), validArea.topRight()), &pTopI);
      anotherLine.intersects(QLineF(validArea.bottomLeft(), validArea.bottomRight()), &pBottomI);
#endif

      if ((pTopI.x() < minXTop) || (pTopI.x() > maxXTop) || (pBottomI.x() < minXBottom)
          || (pBottomI.x() > maxXBottom)) {
        continue;
      }

      if (lineIdx > i) {
        minXTop = pTopI.x();
        minXBottom = pBottomI.x();
      } else {
        maxXTop = pTopI.x();
        maxXBottom = pBottomI.x();
      }
    }
  }

  // Limit movement.
  const double left = qMax(minXTop - pTop.x(), minXBottom - pBottom.x());
  const double right = qMax(pTop.x() - maxXTop, pBottom.x() - maxXBottom);
  if ((left > right) && (left > 0.0)) {
    line.translate(left, 0.0);
  } else if (right > 0.0) {
    line.translate(-right, 0.0);
  }

  m_virtLayout.setCutterLine(lineIdx, widgetToVirtual().map(line));
  update();
}

void ImageView::dragFinished() {
  // When a handle is being dragged, the other handle is displayed not
  // at the edge of the widget widget but at the edge of the image.
  // That means we have to redraw once dragging is over.
  // BTW, the only reason for displaying handles at widget's edges
  // is to make them visible and accessible for dragging.
  update();
  emit pageLayoutSetLocally(m_virtLayout);
}

QPointF ImageView::leftPageCenter() const {
  QRectF leftRect(m_virtLayout.leftPageOutline().boundingRect());
  QRectF rightRect(m_virtLayout.rightPageOutline().boundingRect());

  const double xMid = 0.5 * (leftRect.right() + rightRect.left());
  leftRect.setRight(xMid);
  rightRect.setLeft(xMid);
  return virtualToWidget().map(leftRect.center());
}

QPointF ImageView::rightPageCenter() const {
  QRectF leftRect(m_virtLayout.leftPageOutline().boundingRect());
  QRectF rightRect(m_virtLayout.rightPageOutline().boundingRect());

  const double xMid = 0.5 * (leftRect.right() + rightRect.left());
  leftRect.setRight(xMid);
  rightRect.setLeft(xMid);
  return virtualToWidget().map(rightRect.center());
}

void ImageView::unremoveLeftPage() {
  PageInfo pageInfo(m_pages->unremovePage(PageId(m_imageId, PageId::LEFT_PAGE)));
  m_leftUnremoveButton.unlink();
  m_leftPageRemoved = false;

  update();

  // We need invalidateThumbnail(PageInfo) rather than (PageId),
  // as we are updating page removal status.
  pageInfo.setId(PageId(m_imageId, PageId::SINGLE_PAGE));
  emit invalidateThumbnail(pageInfo);
}

void ImageView::unremoveRightPage() {
  PageInfo pageInfo(m_pages->unremovePage(PageId(m_imageId, PageId::RIGHT_PAGE)));
  m_rightUnremoveButton.unlink();
  m_rightPageRemoved = false;

  update();

  // We need invalidateThumbnail(PageInfo) rather than (PageId),
  // as we are updating page removal status.
  pageInfo.setId(PageId(m_imageId, PageId::SINGLE_PAGE));
  emit invalidateThumbnail(pageInfo);
}
}  // namespace page_split
