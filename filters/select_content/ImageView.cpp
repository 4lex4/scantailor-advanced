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

#include "ImageView.h"
#include <Despeckle.h>
#include <EmptyTaskStatus.h>
#include <TaskStatus.h>
#include <imageproc/Binarize.h>
#include <imageproc/GrayImage.h>
#include <imageproc/PolygonRasterizer.h>
#include <imageproc/Transform.h>
#include <QDebug>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <boost/bind.hpp>
#include <map>
#include "ImagePresentation.h"
#include "ImageTransformation.h"

using namespace imageproc;

namespace select_content {
ImageView::ImageView(const QImage& image,
                     const QImage& downscaled_image,
                     const GrayImage& gray_image,
                     const ImageTransformation& xform,
                     const QRectF& content_rect,
                     const QRectF& page_rect,
                     const bool page_rect_enabled)
    : ImageViewBase(image, downscaled_image, ImagePresentation(xform.transform(), xform.resultingPreCropArea())),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_pNoContentMenu(new QMenu(this)),
      m_pHaveContentMenu(new QMenu(this)),
      m_contentRect(content_rect),
      m_pageRect(page_rect),
      m_minBoxSize(10.0, 10.0),
      m_pageRectEnabled(page_rect_enabled),
      m_pageRectReloadRequested(false) {
  setMouseTracking(true);

  interactionState().setDefaultStatusTip(
      tr("Use the context menu to enable / disable the content box. Hold Shift to drag a box. Use double-click "
         "on content to automatically adjust the content area."));

  const QString content_rect_drag_tip(tr("Drag lines or corners to resize the content box."));
  const QString page_rect_drag_tip(tr("Drag lines or corners to resize the page box."));
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
    m_contentRectCornerHandlers[i].setProximityStatusTip(content_rect_drag_tip);
    m_pageRectCorners[i].setPositionCallback(boost::bind(&ImageView::pageRectCornerPosition, this, masks_by_corner[i]));
    m_pageRectCorners[i].setMoveRequestCallback(
        boost::bind(&ImageView::pageRectCornerMoveRequest, this, masks_by_corner[i], _1));
    m_pageRectCorners[i].setDragFinishedCallback(boost::bind(&ImageView::pageRectDragFinished, this));
    m_pageRectCornerHandlers[i].setObject(&m_pageRectCorners[i]);
    m_pageRectCornerHandlers[i].setProximityStatusTip(page_rect_drag_tip);

    // Setup edge drag handlers.
    m_contentRectEdges[i].setPositionCallback(boost::bind(&ImageView::contentRectEdgePosition, this, masks_by_edge[i]));
    m_contentRectEdges[i].setMoveRequestCallback(
        boost::bind(&ImageView::contentRectEdgeMoveRequest, this, masks_by_edge[i], _1));
    m_contentRectEdges[i].setDragFinishedCallback(boost::bind(&ImageView::contentRectDragFinished, this));
    m_contentRectEdgeHandlers[i].setObject(&m_contentRectEdges[i]);
    m_contentRectEdgeHandlers[i].setProximityStatusTip(content_rect_drag_tip);
    m_pageRectEdges[i].setPositionCallback(boost::bind(&ImageView::pageRectEdgePosition, this, masks_by_edge[i]));
    m_pageRectEdges[i].setMoveRequestCallback(
        boost::bind(&ImageView::pageRectEdgeMoveRequest, this, masks_by_edge[i], _1));
    m_pageRectEdges[i].setDragFinishedCallback(boost::bind(&ImageView::pageRectDragFinished, this));
    m_pageRectEdgeHandlers[i].setObject(&m_pageRectEdges[i]);
    m_pageRectEdgeHandlers[i].setProximityStatusTip(page_rect_drag_tip);

    Qt::CursorShape corner_cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
    m_contentRectCornerHandlers[i].setProximityCursor(corner_cursor);
    m_contentRectCornerHandlers[i].setInteractionCursor(corner_cursor);
    m_pageRectCornerHandlers[i].setProximityCursor(corner_cursor);
    m_pageRectCornerHandlers[i].setInteractionCursor(corner_cursor);

    Qt::CursorShape edge_cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
    m_contentRectEdgeHandlers[i].setProximityCursor(edge_cursor);
    m_contentRectEdgeHandlers[i].setInteractionCursor(edge_cursor);
    m_pageRectEdgeHandlers[i].setProximityCursor(edge_cursor);
    m_pageRectEdgeHandlers[i].setInteractionCursor(edge_cursor);

    makeLastFollower(m_contentRectCornerHandlers[i]);
    makeLastFollower(m_contentRectEdgeHandlers[i]);
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

    makeLastFollower(m_contentRectAreaHandler);
    if (m_pageRectEnabled) {
      makeLastFollower(m_pageRectAreaHandler);
    }
  }

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  QAction* create = m_pNoContentMenu->addAction(tr("Create Content Box"));
  QAction* remove = m_pHaveContentMenu->addAction(tr("Remove Content Box"));
  create->setShortcut(QKeySequence("Ins"));
  remove->setShortcut(QKeySequence("Delete"));
  addAction(create);
  addAction(remove);
  connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
  connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));

  buildContentImage(gray_image, xform);
}

ImageView::~ImageView() = default;

void ImageView::createContentBox() {
  if (!m_contentRect.isEmpty()) {
    return;
  }
  if (interactionState().captured()) {
    return;
  }

  const QRectF virtual_rect(virtualDisplayRect());
  QRectF content_rect(0, 0, virtual_rect.width() * 0.7, virtual_rect.height() * 0.7);
  content_rect.moveCenter(virtual_rect.center());
  m_contentRect = content_rect;
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
    m_pNoContentMenu->popup(event->globalPos());
  } else {
    m_pHaveContentMenu->popup(event->globalPos());
  }
}

QPointF ImageView::contentRectCornerPosition(int edge_mask) const {
  const QRectF rect(virtualToWidget().mapRect(m_contentRect));
  QPointF pt;

  if (edge_mask & TOP) {
    pt.setY(rect.top());
  } else if (edge_mask & BOTTOM) {
    pt.setY(rect.bottom());
  }

  if (edge_mask & LEFT) {
    pt.setX(rect.left());
  } else if (edge_mask & RIGHT) {
    pt.setX(rect.right());
  }

  return pt;
}

void ImageView::contentRectCornerMoveRequest(int edge_mask, const QPointF& pos) {
  QRectF r(virtualToWidget().mapRect(m_contentRect));
  const qreal minw = m_minBoxSize.width();
  const qreal minh = m_minBoxSize.height();

  if (edge_mask & TOP) {
    r.setTop(std::min(pos.y(), r.bottom() - minh));
  } else if (edge_mask & BOTTOM) {
    r.setBottom(std::max(pos.y(), r.top() + minh));
  }

  if (edge_mask & LEFT) {
    r.setLeft(std::min(pos.x(), r.right() - minw));
  } else if (edge_mask & RIGHT) {
    r.setRight(std::max(pos.x(), r.left() + minw));
  }

  forceInsideImage(r, edge_mask);
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

QPointF ImageView::pageRectCornerPosition(int edge_mask) const {
  const QRectF rect(virtualToWidget().mapRect(m_pageRect));
  QPointF pt;

  if (edge_mask & TOP) {
    pt.setY(rect.top());
  } else if (edge_mask & BOTTOM) {
    pt.setY(rect.bottom());
  }

  if (edge_mask & LEFT) {
    pt.setX(rect.left());
  } else if (edge_mask & RIGHT) {
    pt.setX(rect.right());
  }

  return pt;
}

void ImageView::pageRectCornerMoveRequest(int edge_mask, const QPointF& pos) {
  QRectF r(virtualToWidget().mapRect(m_pageRect));
  const qreal minw = m_minBoxSize.width();
  const qreal minh = m_minBoxSize.height();

  if (edge_mask & TOP) {
    r.setTop(std::min(pos.y(), r.bottom() - minh));
  } else if (edge_mask & BOTTOM) {
    r.setBottom(std::max(pos.y(), r.top() + minh));
  }

  if (edge_mask & LEFT) {
    r.setLeft(std::min(pos.x(), r.right() - minw));
  } else if (edge_mask & RIGHT) {
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

void ImageView::forceInsideImage(QRectF& widget_rect, const int edge_mask) const {
  const qreal minw = m_minBoxSize.width();
  const qreal minh = m_minBoxSize.height();
  const QRectF image_rect(virtualToWidget().mapRect(virtualDisplayRect()));

  if ((edge_mask & LEFT) && (widget_rect.left() < image_rect.left())) {
    widget_rect.setLeft(image_rect.left());
    widget_rect.setRight(std::max(widget_rect.right(), widget_rect.left() + minw));
  }
  if ((edge_mask & RIGHT) && (widget_rect.right() > image_rect.right())) {
    widget_rect.setRight(image_rect.right());
    widget_rect.setLeft(std::min(widget_rect.left(), widget_rect.right() - minw));
  }
  if ((edge_mask & TOP) && (widget_rect.top() < image_rect.top())) {
    widget_rect.setTop(image_rect.top());
    widget_rect.setBottom(std::max(widget_rect.bottom(), widget_rect.top() + minh));
  }
  if ((edge_mask & BOTTOM) && (widget_rect.bottom() > image_rect.bottom())) {
    widget_rect.setBottom(image_rect.bottom());
    widget_rect.setTop(std::min(widget_rect.top(), widget_rect.bottom() - minh));
  }
}

void ImageView::forcePageRectDescribeContent() {
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

void ImageView::contentRectMoveRequest(const QPolygonF& poly_moved) {
  QRectF contentRectInWidget(poly_moved.boundingRect());

  const QRectF image_rect(virtualToWidget().mapRect(virtualDisplayRect()));
  if (contentRectInWidget.left() < image_rect.left()) {
    contentRectInWidget.translate(image_rect.left() - contentRectInWidget.left(), 0);
  }
  if (contentRectInWidget.right() > image_rect.right()) {
    contentRectInWidget.translate(image_rect.right() - contentRectInWidget.right(), 0);
  }
  if (contentRectInWidget.top() < image_rect.top()) {
    contentRectInWidget.translate(0, image_rect.top() - contentRectInWidget.top());
  }
  if (contentRectInWidget.bottom() > image_rect.bottom()) {
    contentRectInWidget.translate(0, image_rect.bottom() - contentRectInWidget.bottom());
  }

  m_contentRect = widgetToVirtual().mapRect(contentRectInWidget);

  forcePageRectDescribeContent();

  update();
}

QRectF ImageView::pageRectPosition() const {
  return virtualToWidget().mapRect(m_pageRect);
}

void ImageView::pageRectMoveRequest(const QPolygonF& poly_moved) {
  QRectF pageRectInWidget(poly_moved.boundingRect());

  const QRectF content_rect(virtualToWidget().mapRect(m_contentRect));
  if (pageRectInWidget.left() > content_rect.left()) {
    pageRectInWidget.translate(content_rect.left() - pageRectInWidget.left(), 0);
  }
  if (pageRectInWidget.right() < content_rect.right()) {
    pageRectInWidget.translate(content_rect.right() - pageRectInWidget.right(), 0);
  }
  if (pageRectInWidget.top() > content_rect.top()) {
    pageRectInWidget.translate(0, content_rect.top() - pageRectInWidget.top());
  }
  if (pageRectInWidget.bottom() < content_rect.bottom()) {
    pageRectInWidget.translate(0, content_rect.bottom() - pageRectInWidget.bottom());
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

  update();
  emit manualPageRectSet(m_pageRect);
}

void ImageView::buildContentImage(const GrayImage& gray_image, const ImageTransformation& xform) {
  ImageTransformation xform_150dpi(xform);
  xform_150dpi.preScaleToDpi(Dpi(150, 150));

  if (xform_150dpi.resultingRect().toRect().isEmpty()) {
    return;
  }

  QImage gray150(transformToGray(gray_image, xform_150dpi.transform(), xform_150dpi.resultingRect().toRect(),
                                 OutsidePixels::assumeColor(Qt::white)));

  m_contentImage = binarizeWolf(gray150, QSize(51, 51), 50);

  PolygonRasterizer::fillExcept(m_contentImage, WHITE, xform_150dpi.resultingPreCropArea(), Qt::WindingFill);

  Despeckle::despeckleInPlace(m_contentImage, Dpi(150, 150), Despeckle::NORMAL, EmptyTaskStatus());

  m_originalToContentImage = xform_150dpi.transform();
  m_contentImageToOriginal = m_originalToContentImage.inverted();
}

void ImageView::onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() == Qt::LeftButton) {
    if (!m_contentRect.isEmpty() && !m_contentImage.isNull()) {
      correctContentBox(QPointF(0.5, 0.5) + event->pos());
    }
  }
}

void ImageView::correctContentBox(const QPointF& pos) {
  const QTransform widget_to_content_image(widgetToImage() * m_originalToContentImage);
  const QTransform content_image_to_virtual(m_contentImageToOriginal * imageToVirtual());

  const QPointF content_pos = widget_to_content_image.map(pos);

  QRect finding_area((content_pos - QPointF(15, 15)).toPoint(), QSize(30, 30));
  finding_area = finding_area.intersected(m_contentImage.rect());
  if (finding_area.isEmpty()) {
    return;
  }

  QRect found_area = findContentInArea(finding_area);
  if (found_area.isEmpty()) {
    return;
  }

  // If click position is inside the content rect, adjust the nearest side of the rect,
  // else include the content at the position into the content rect.
  const QPointF pos_in_virtual = widgetToVirtual().map(pos);
  const QRectF found_area_in_virtual = content_image_to_virtual.mapRect(QRectF(found_area));
  if (!m_contentRect.contains(pos_in_virtual)) {
    m_contentRect |= found_area_in_virtual;
    forcePageRectDescribeContent();
  } else {
    std::map<double, Edge> distanceMap;
    distanceMap[pos_in_virtual.y() - m_contentRect.top()] = TOP;
    distanceMap[pos_in_virtual.x() - m_contentRect.left()] = LEFT;
    distanceMap[m_contentRect.bottom() - pos_in_virtual.y()] = BOTTOM;
    distanceMap[m_contentRect.right() - pos_in_virtual.x()] = RIGHT;

    const Edge edge = distanceMap.begin()->second;
    QPointF movePoint;
    switch (edge) {
      case TOP:
      case LEFT:
        movePoint = QPointF(found_area_in_virtual.left(), found_area_in_virtual.top());
        break;
      case BOTTOM:
      case RIGHT:
        movePoint = QPointF(found_area_in_virtual.right(), found_area_in_virtual.bottom());
        break;
    }

    contentRectCornerMoveRequest(edge, virtualToWidget().map(movePoint));
  }

  update();
  contentRectDragFinished();
}

QRect ImageView::findContentInArea(const QRect& area) const {
  const uint32_t* image_line = m_contentImage.data();
  const int image_stride = m_contentImage.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  int top = std::numeric_limits<int>::max();
  int left = std::numeric_limits<int>::max();
  int bottom = std::numeric_limits<int>::min();
  int right = std::numeric_limits<int>::min();

  image_line += area.top() * image_stride;
  for (int y = area.top(); y <= area.bottom(); ++y) {
    for (int x = area.left(); x <= area.right(); ++x) {
      if (image_line[x >> 5] & (msb >> (x & 31))) {
        top = std::min(top, y);
        left = std::min(left, x);
        bottom = std::max(bottom, y);
        right = std::max(right, x);
      }
    }
    image_line += image_stride;
  }

  if (top > bottom) {
    return QRect();
  }

  QRect found_area = QRect(left, top, right - left + 1, bottom - top + 1);
  found_area.adjust(-1, -1, 1, 1);

  return found_area;
}

void ImageView::setPageRectEnabled(const bool state) {
  m_pageRectEnabled = state;

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
  };

  update();
}
}  // namespace select_content