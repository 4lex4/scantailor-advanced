// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageView.h"

#include <TaskStatus.h>

#include <QDebug>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <boost/bind.hpp>
#include <cmath>
#include <map>

#include "ImagePresentation.h"
#include "ImageTransformation.h"

using namespace imageproc;

namespace select_content {
ImageView::ImageView(const QImage& image,
                     const QImage& downscaledImage,
                     const ContentMask& contentMask,
                     const ImageTransformation& xform,
                     const QRectF& contentRect,
                     const QRectF& pageRect,
                     const bool pageRectEnabled)
    : ImageViewBase(image, downscaledImage, ImagePresentation(xform.transform(), xform.resultingPreCropArea())),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_noContentMenu(new QMenu(this)),
      m_haveContentMenu(new QMenu(this)),
      m_contentRect(contentRect),
      m_pageRect(pageRect),
      m_pageRectEnabled(pageRectEnabled),
      m_pageRectReloadRequested(false),
      m_minBoxSize(10.0, 10.0),
      m_contentMask(contentMask),
      m_adjustmentVerticalModifier(Qt::ControlModifier),
      m_adjustmentHorizontalModifier(Qt::ShiftModifier) {
  setMouseTracking(true);

  interactionState().setDefaultStatusTip(
      tr("Use the context menu to enable / disable the content box. Hold Shift to drag a box. Use double-click "
         "on content to automatically adjust the content area."));

  const QString contentRectDragTip(tr("Drag lines or corners to resize the content box."));
  const QString pageRectDragTip(tr("Drag lines or corners to resize the page box."));
  static const int masks_by_edge[] = {TOP, RIGHT, BOTTOM, LEFT};
  static const int masks_by_corner[] = {TOP | LEFT, TOP | RIGHT, BOTTOM | RIGHT, BOTTOM | LEFT};
  for (int i = 0; i < 4; ++i) {
    // Proximity priority - content rect higher than page, corners higher than edges.
    m_contentRectCorners[i].setProximityPriority(4);
    m_contentRectEdges[i].setProximityPriority(3);
    m_pageRectCorners[i].setProximityPriority(2);
    m_pageRectEdges[i].setProximityPriority(1);

    // Setup corner drag handlers.
    m_contentRectCorners[i].setPositionCallback(
        boost::bind(&ImageView::contentRectCornerPosition, this, masks_by_corner[i]));
    m_contentRectCorners[i].setMoveRequestCallback(
        boost::bind(&ImageView::contentRectCornerMoveRequest, this, masks_by_corner[i], _1));
    m_contentRectCorners[i].setDragFinishedCallback(boost::bind(&ImageView::contentRectDragFinished, this));
    m_contentRectCornerHandlers[i].setObject(&m_contentRectCorners[i]);
    m_contentRectCornerHandlers[i].setProximityStatusTip(contentRectDragTip);
    m_pageRectCorners[i].setPositionCallback(boost::bind(&ImageView::pageRectCornerPosition, this, masks_by_corner[i]));
    m_pageRectCorners[i].setMoveRequestCallback(
        boost::bind(&ImageView::pageRectCornerMoveRequest, this, masks_by_corner[i], _1));
    m_pageRectCorners[i].setDragFinishedCallback(boost::bind(&ImageView::pageRectDragFinished, this));
    m_pageRectCornerHandlers[i].setObject(&m_pageRectCorners[i]);
    m_pageRectCornerHandlers[i].setProximityStatusTip(pageRectDragTip);

    // Setup edge drag handlers.
    m_contentRectEdges[i].setPositionCallback(boost::bind(&ImageView::contentRectEdgePosition, this, masks_by_edge[i]));
    m_contentRectEdges[i].setMoveRequestCallback(
        boost::bind(&ImageView::contentRectEdgeMoveRequest, this, masks_by_edge[i], _1));
    m_contentRectEdges[i].setDragFinishedCallback(boost::bind(&ImageView::contentRectDragFinished, this));
    m_contentRectEdgeHandlers[i].setObject(&m_contentRectEdges[i]);
    m_contentRectEdgeHandlers[i].setProximityStatusTip(contentRectDragTip);
    m_pageRectEdges[i].setPositionCallback(boost::bind(&ImageView::pageRectEdgePosition, this, masks_by_edge[i]));
    m_pageRectEdges[i].setMoveRequestCallback(
        boost::bind(&ImageView::pageRectEdgeMoveRequest, this, masks_by_edge[i], _1));
    m_pageRectEdges[i].setDragFinishedCallback(boost::bind(&ImageView::pageRectDragFinished, this));
    m_pageRectEdgeHandlers[i].setObject(&m_pageRectEdges[i]);
    m_pageRectEdgeHandlers[i].setProximityStatusTip(pageRectDragTip);

    Qt::CursorShape cornerCursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
    m_contentRectCornerHandlers[i].setProximityCursor(cornerCursor);
    m_contentRectCornerHandlers[i].setInteractionCursor(cornerCursor);
    m_pageRectCornerHandlers[i].setProximityCursor(cornerCursor);
    m_pageRectCornerHandlers[i].setInteractionCursor(cornerCursor);

    Qt::CursorShape edgeCursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
    m_contentRectEdgeHandlers[i].setProximityCursor(edgeCursor);
    m_contentRectEdgeHandlers[i].setInteractionCursor(edgeCursor);
    m_pageRectEdgeHandlers[i].setProximityCursor(edgeCursor);
    m_pageRectEdgeHandlers[i].setInteractionCursor(edgeCursor);

    if (m_contentRect.isValid()) {
      makeLastFollower(m_contentRectCornerHandlers[i]);
      makeLastFollower(m_contentRectEdgeHandlers[i]);
    }
    if (m_pageRectEnabled) {
      makeLastFollower(m_pageRectCornerHandlers[i]);
      makeLastFollower(m_pageRectEdgeHandlers[i]);
    }
  }

  {
    // Proximity priority - content rect higher than page
    m_contentRectArea.setProximityPriority(2);
    m_pageRectArea.setProximityPriority(1);

    // Setup rectangle drag interaction
    m_contentRectArea.setPositionCallback(boost::bind(&ImageView::contentRectPosition, this));
    m_contentRectArea.setMoveRequestCallback(boost::bind(&ImageView::contentRectMoveRequest, this, _1));
    m_contentRectArea.setDragFinishedCallback(boost::bind(&ImageView::contentRectDragFinished, this));
    m_contentRectAreaHandler.setObject(&m_contentRectArea);
    m_contentRectAreaHandler.setProximityStatusTip(tr("Hold left mouse button to drag the content box."));
    m_contentRectAreaHandler.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));
    m_pageRectArea.setPositionCallback(boost::bind(&ImageView::pageRectPosition, this));
    m_pageRectArea.setMoveRequestCallback(boost::bind(&ImageView::pageRectMoveRequest, this, _1));
    m_pageRectArea.setDragFinishedCallback(boost::bind(&ImageView::pageRectDragFinished, this));
    m_pageRectAreaHandler.setObject(&m_pageRectArea);
    m_pageRectAreaHandler.setProximityStatusTip(tr("Hold left mouse button to drag the page box."));
    m_pageRectAreaHandler.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));

    Qt::CursorShape cursor = Qt::DragMoveCursor;
    m_contentRectAreaHandler.setKeyboardModifiers({Qt::ShiftModifier});
    m_contentRectAreaHandler.setProximityCursor(cursor);
    m_contentRectAreaHandler.setInteractionCursor(cursor);
    m_pageRectAreaHandler.setKeyboardModifiers({Qt::ShiftModifier});
    m_pageRectAreaHandler.setProximityCursor(cursor);
    m_pageRectAreaHandler.setInteractionCursor(cursor);

    if (m_contentRect.isValid()) {
      makeLastFollower(m_contentRectAreaHandler);
    }
    if (m_pageRectEnabled) {
      makeLastFollower(m_pageRectAreaHandler);
    }
  }

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  QAction* create = m_noContentMenu->addAction(tr("Create Content Box"));
  QAction* remove = m_haveContentMenu->addAction(tr("Remove Content Box"));
  create->setShortcut(QKeySequence("Ins"));
  remove->setShortcut(QKeySequence("Delete"));
  addAction(create);
  addAction(remove);
  connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
  connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));
}

ImageView::~ImageView() = default;

void ImageView::createContentBox() {
  if (!m_contentRect.isEmpty()) {
    return;
  }
  if (interactionState().captured()) {
    return;
  }

  const QRectF virtualRect(virtualDisplayRect());
  QRectF contentRect(0, 0, virtualRect.width() * 0.7, virtualRect.height() * 0.7);
  contentRect.moveCenter(virtualRect.center());
  m_contentRect = contentRect;
  forcePageRectDescribeContent();
  enableContentRectInteraction(true);

  update();
  emit manualContentRectSet(m_contentRect);
}

void ImageView::removeContentBox() {
  if (m_contentRect.isEmpty()) {
    return;
  }
  if (interactionState().captured()) {
    return;
  }

  enableContentRectInteraction(false);
  m_contentRect = QRectF();

  update();
  emit manualContentRectSet(m_contentRect);
}

void ImageView::onPaint(QPainter& painter, const InteractionState& interaction) {
  if (m_contentRect.isNull() && !m_pageRectEnabled) {
    return;
  }

  painter.setRenderHints(QPainter::Antialiasing, true);

  if (m_pageRectEnabled) {
    // Draw the page bounding box.
    QPen pen(QColor(0xff, 0x7f, 0x00));
    pen.setWidthF(2.0);
    pen.setCosmetic(true);
    painter.setPen(pen);

    painter.setBrush(Qt::NoBrush);

    painter.drawRect(m_pageRect);
  }

  if (m_contentRect.isNull()) {
    return;
  }

  // Draw the content bounding box.
  QPen pen(QColor(0x00, 0x00, 0xff));
  pen.setWidthF(1.0);
  pen.setCosmetic(true);
  painter.setPen(pen);

  painter.setBrush(QColor(0x00, 0x00, 0xff, 50));

  // Pen strokes will be outside of m_contentRect - that's how drawRect() works.
  painter.drawRect(m_contentRect);
}  // ImageView::onPaint

void ImageView::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {
  if (interaction.captured()) {
    // No context menus during resizing.
    return;
  }

  if (m_contentRect.isEmpty()) {
    m_noContentMenu->popup(event->globalPos());
  } else {
    m_haveContentMenu->popup(event->globalPos());
  }
}

QPointF ImageView::contentRectCornerPosition(int edgeMask) const {
  const QRectF rect(virtualToWidget().mapRect(m_contentRect));
  QPointF pt;

  if (edgeMask & TOP) {
    pt.setY(rect.top());
  } else if (edgeMask & BOTTOM) {
    pt.setY(rect.bottom());
  }

  if (edgeMask & LEFT) {
    pt.setX(rect.left());
  } else if (edgeMask & RIGHT) {
    pt.setX(rect.right());
  }
  return pt;
}

void ImageView::contentRectCornerMoveRequest(int edgeMask, const QPointF& pos) {
  QRectF r(virtualToWidget().mapRect(m_contentRect));
  const qreal minw = m_minBoxSize.width();
  const qreal minh = m_minBoxSize.height();

  if (edgeMask & TOP) {
    r.setTop(std::min(pos.y(), r.bottom() - minh));
  } else if (edgeMask & BOTTOM) {
    r.setBottom(std::max(pos.y(), r.top() + minh));
  }

  if (edgeMask & LEFT) {
    r.setLeft(std::min(pos.x(), r.right() - minw));
  } else if (edgeMask & RIGHT) {
    r.setRight(std::max(pos.x(), r.left() + minw));
  }

  forceInsideImage(r, edgeMask);
  m_contentRect = widgetToVirtual().mapRect(r);

  forcePageRectDescribeContent();

  update();
}

QLineF ImageView::contentRectEdgePosition(int edge) const {
  const QRectF rect(virtualToWidget().mapRect(m_contentRect));

  if (edge == TOP) {
    return QLineF(rect.topLeft(), rect.topRight());
  } else if (edge == BOTTOM) {
    return QLineF(rect.bottomLeft(), rect.bottomRight());
  } else if (edge == LEFT) {
    return QLineF(rect.topLeft(), rect.bottomLeft());
  } else {
    return QLineF(rect.topRight(), rect.bottomRight());
  }
}

void ImageView::contentRectEdgeMoveRequest(int edge, const QLineF& line) {
  contentRectCornerMoveRequest(edge, line.p1());
}

void ImageView::contentRectDragFinished() {
  emit manualContentRectSet(m_contentRect);
  if (m_pageRectReloadRequested) {
    emit manualPageRectSet(m_pageRect);
    m_pageRectReloadRequested = false;
  }
}

QPointF ImageView::pageRectCornerPosition(int edgeMask) const {
  const QRectF rect(virtualToWidget().mapRect(m_pageRect));
  QPointF pt;

  if (edgeMask & TOP) {
    pt.setY(rect.top());
  } else if (edgeMask & BOTTOM) {
    pt.setY(rect.bottom());
  }

  if (edgeMask & LEFT) {
    pt.setX(rect.left());
  } else if (edgeMask & RIGHT) {
    pt.setX(rect.right());
  }
  return pt;
}

void ImageView::pageRectCornerMoveRequest(int edgeMask, const QPointF& pos) {
  QRectF r(virtualToWidget().mapRect(m_pageRect));
  const qreal minw = m_minBoxSize.width();
  const qreal minh = m_minBoxSize.height();

  if (edgeMask & TOP) {
    r.setTop(std::min(pos.y(), r.bottom() - minh));
  } else if (edgeMask & BOTTOM) {
    r.setBottom(std::max(pos.y(), r.top() + minh));
  }

  if (edgeMask & LEFT) {
    r.setLeft(std::min(pos.x(), r.right() - minw));
  } else if (edgeMask & RIGHT) {
    r.setRight(std::max(pos.x(), r.left() + minw));
  }

  m_pageRect = widgetToVirtual().mapRect(r);
  forcePageRectDescribeContent();

  update();
  emit pageRectSizeChanged(m_pageRect.size());
}

QLineF ImageView::pageRectEdgePosition(int edge) const {
  const QRectF rect(virtualToWidget().mapRect(m_pageRect));

  if (edge == TOP) {
    return QLineF(rect.topLeft(), rect.topRight());
  } else if (edge == BOTTOM) {
    return QLineF(rect.bottomLeft(), rect.bottomRight());
  } else if (edge == LEFT) {
    return QLineF(rect.topLeft(), rect.bottomLeft());
  } else {
    return QLineF(rect.topRight(), rect.bottomRight());
  }
}

void ImageView::pageRectEdgeMoveRequest(int edge, const QLineF& line) {
  pageRectCornerMoveRequest(edge, line.p1());
}

void ImageView::pageRectDragFinished() {
  emit manualPageRectSet(m_pageRect);
}

void ImageView::forceInsideImage(QRectF& widgetRect, const int edgeMask) const {
  const qreal minw = m_minBoxSize.width();
  const qreal minh = m_minBoxSize.height();
  const QRectF imageRect(virtualToWidget().mapRect(virtualDisplayRect()));

  if ((edgeMask & LEFT) && (widgetRect.left() < imageRect.left())) {
    widgetRect.setLeft(imageRect.left());
    widgetRect.setRight(std::max(widgetRect.right(), widgetRect.left() + minw));
  }
  if ((edgeMask & RIGHT) && (widgetRect.right() > imageRect.right())) {
    widgetRect.setRight(imageRect.right());
    widgetRect.setLeft(std::min(widgetRect.left(), widgetRect.right() - minw));
  }
  if ((edgeMask & TOP) && (widgetRect.top() < imageRect.top())) {
    widgetRect.setTop(imageRect.top());
    widgetRect.setBottom(std::max(widgetRect.bottom(), widgetRect.top() + minh));
  }
  if ((edgeMask & BOTTOM) && (widgetRect.bottom() > imageRect.bottom())) {
    widgetRect.setBottom(imageRect.bottom());
    widgetRect.setTop(std::min(widgetRect.top(), widgetRect.bottom() - minh));
  }
}

void ImageView::forcePageRectDescribeContent() {
  if (!m_contentRect.isValid()) {
    return;
  }

  const QRectF oldPageRect = m_pageRect;
  m_pageRect |= m_contentRect;
  if (m_pageRectEnabled && (m_pageRect != oldPageRect)) {
    m_pageRectReloadRequested = true;
    emit pageRectSizeChanged(m_pageRect.size());
  }
}

QRectF ImageView::contentRectPosition() const {
  return virtualToWidget().mapRect(m_contentRect);
}

void ImageView::contentRectMoveRequest(const QPolygonF& polyMoved) {
  QRectF contentRectInWidget(polyMoved.boundingRect());

  const QRectF imageRect(virtualToWidget().mapRect(virtualDisplayRect()));
  if (contentRectInWidget.left() < imageRect.left()) {
    contentRectInWidget.translate(imageRect.left() - contentRectInWidget.left(), 0);
  }
  if (contentRectInWidget.right() > imageRect.right()) {
    contentRectInWidget.translate(imageRect.right() - contentRectInWidget.right(), 0);
  }
  if (contentRectInWidget.top() < imageRect.top()) {
    contentRectInWidget.translate(0, imageRect.top() - contentRectInWidget.top());
  }
  if (contentRectInWidget.bottom() > imageRect.bottom()) {
    contentRectInWidget.translate(0, imageRect.bottom() - contentRectInWidget.bottom());
  }

  m_contentRect = widgetToVirtual().mapRect(contentRectInWidget);

  forcePageRectDescribeContent();

  update();
}

QRectF ImageView::pageRectPosition() const {
  return virtualToWidget().mapRect(m_pageRect);
}

void ImageView::pageRectMoveRequest(const QPolygonF& polyMoved) {
  QRectF pageRectInWidget(polyMoved.boundingRect());

  if (m_contentRect.isValid()) {
    const QRectF contentRect(virtualToWidget().mapRect(m_contentRect));
    if (pageRectInWidget.left() > contentRect.left()) {
      pageRectInWidget.translate(contentRect.left() - pageRectInWidget.left(), 0);
    }
    if (pageRectInWidget.right() < contentRect.right()) {
      pageRectInWidget.translate(contentRect.right() - pageRectInWidget.right(), 0);
    }
    if (pageRectInWidget.top() > contentRect.top()) {
      pageRectInWidget.translate(0, contentRect.top() - pageRectInWidget.top());
    }
    if (pageRectInWidget.bottom() < contentRect.bottom()) {
      pageRectInWidget.translate(0, contentRect.bottom() - pageRectInWidget.bottom());
    }
  }

  m_pageRect = widgetToVirtual().mapRect(pageRectInWidget);

  update();
}

void ImageView::pageRectSetExternally(const QRectF& pageRect) {
  if (!m_pageRectEnabled) {
    return;
  }
  m_pageRect = pageRect;

  forcePageRectDescribeContent();
  if (m_pageRectReloadRequested) {
    emit manualPageRectSet(m_pageRect);
  }

  update();
}

void ImageView::onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() == Qt::LeftButton) {
    if (!m_contentRect.isEmpty() && !m_contentMask.isNull()) {
      correctContentBox(QPointF(0.5, 0.5) + event->pos(), event->modifiers());
    }
  }
}

void ImageView::correctContentBox(const QPointF& pos, const Qt::KeyboardModifiers mask) {
  const QTransform widgetToContentImage(widgetToImage() * m_contentMask.originalToContentXform());
  const QTransform contentImageToVirtual(m_contentMask.contentToOriginalXform() * imageToVirtual());

  const QPointF contentPos = widgetToContentImage.map(pos);
  QRect findingArea((contentPos - QPointF(15, 15)).toPoint(), QSize(30, 30));
  QRect foundArea = m_contentMask.findContentInArea(findingArea);
  if (foundArea.isEmpty()) {
    return;
  }

  const QPointF posInVirtual = widgetToVirtual().map(pos);
  const QRectF foundAreaInVirtual = contentImageToVirtual.mapRect(QRectF(foundArea)).intersected(virtualDisplayRect());
  if (foundAreaInVirtual.isEmpty()) {
    return;
  }

  // If click position is inside the content rect, adjust the nearest side of the rect,
  // else include the content at the position into the content rect.
  if (!m_contentRect.contains(posInVirtual)) {
    m_contentRect |= foundAreaInVirtual;
    forcePageRectDescribeContent();
  } else {
    const bool onlyHorizontalDirection = (mask == m_adjustmentHorizontalModifier);
    const bool onlyVerticalDirection = (mask == m_adjustmentVerticalModifier);
    const bool bothDirections = (mask == (m_adjustmentVerticalModifier | m_adjustmentHorizontalModifier));

    const double topDist = posInVirtual.y() - m_contentRect.top();
    const double leftDist = posInVirtual.x() - m_contentRect.left();
    const double bottomDist = m_contentRect.bottom() - posInVirtual.y();
    const double rightDist = m_contentRect.right() - posInVirtual.x();
    std::map<double, Edge> distanceMap;
    distanceMap[topDist] = TOP;
    distanceMap[leftDist] = LEFT;
    distanceMap[bottomDist] = BOTTOM;
    distanceMap[rightDist] = RIGHT;

    int edgeMask;
    if (onlyHorizontalDirection) {
      edgeMask = distanceMap.at(std::min(leftDist, rightDist));
    } else if (onlyVerticalDirection) {
      edgeMask = distanceMap.at(std::min(topDist, bottomDist));
    } else if (bothDirections) {
      edgeMask = distanceMap.at(std::min(leftDist, rightDist)) | distanceMap.at(std::min(topDist, bottomDist));
    } else {
      edgeMask = distanceMap.begin()->second;
    }

    QPointF movePoint;
    if (edgeMask & TOP) {
      movePoint.setY(foundAreaInVirtual.top());
    } else if (edgeMask & BOTTOM) {
      movePoint.setY(foundAreaInVirtual.bottom());
    }
    if (edgeMask & LEFT) {
      movePoint.setX(foundAreaInVirtual.left());
    } else if (edgeMask & RIGHT) {
      movePoint.setX(foundAreaInVirtual.right());
    }

    contentRectCornerMoveRequest(edgeMask, virtualToWidget().map(movePoint));
  }

  update();
  contentRectDragFinished();
}

void ImageView::setPageRectEnabled(const bool state) {
  m_pageRectEnabled = state;
  enablePageRectInteraction(state);
  update();
}

void ImageView::enableContentRectInteraction(const bool state) {
  if (state) {
    for (int i = 0; i < 4; ++i) {
      makeLastFollower(m_contentRectCornerHandlers[i]);
      makeLastFollower(m_contentRectEdgeHandlers[i]);
    }
    makeLastFollower(m_contentRectAreaHandler);
  } else {
    for (int i = 0; i < 4; ++i) {
      m_contentRectCornerHandlers[i].unlink();
      m_contentRectEdgeHandlers[i].unlink();
    }
    m_contentRectAreaHandler.unlink();
  }
}

void ImageView::enablePageRectInteraction(const bool state) {
  if (state) {
    for (int i = 0; i < 4; ++i) {
      makeLastFollower(m_pageRectCornerHandlers[i]);
      makeLastFollower(m_pageRectEdgeHandlers[i]);
    }
    makeLastFollower(m_pageRectAreaHandler);
  } else {
    for (int i = 0; i < 4; ++i) {
      m_pageRectCornerHandlers[i].unlink();
      m_pageRectEdgeHandlers[i].unlink();
    }
    m_pageRectAreaHandler.unlink();
  }
}
}  // namespace select_content