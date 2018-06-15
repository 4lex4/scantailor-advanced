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
#include <QDebug>
#include <QPainter>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <utility>
#include "ImagePresentation.h"
#include "ImageTransformation.h"
#include "ProjectPages.h"

namespace page_split {
ImageView::ImageView(const QImage& image,
                     const QImage& downscaled_image,
                     const ImageTransformation& xform,
                     const PageLayout& layout,
                     intrusive_ptr<ProjectPages> pages,
                     const ImageId& image_id,
                     bool left_half_removed,
                     bool right_half_removed)
    : ImageViewBase(image, downscaled_image, ImagePresentation(xform.transform(), xform.resultingPreCropArea())),
      m_ptrPages(std::move(pages)),
      m_imageId(image_id),
      m_leftUnremoveButton(boost::bind(&ImageView::leftPageCenter, this)),
      m_rightUnremoveButton(boost::bind(&ImageView::rightPageCenter, this)),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_handlePixmap(":/icons/aqua-sphere.png"),
      m_virtLayout(layout),
      m_leftPageRemoved(left_half_removed),
      m_rightPageRemoved(right_half_removed) {
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
  const double hit_radius = std::max<double>(0.5 * m_handlePixmap.width(), 15.0);
  const int num_cutters = m_virtLayout.numCutters();
  for (int i = 0; i < num_cutters; ++i) {  // Loop over lines.
    m_lineInteractors[i].setObject(&m_lineSegments[0]);

    for (int j = 0; j < 2; ++j) {  // Loop over handles.
      m_handles[i][j].setHitRadius(hit_radius);
      m_handles[i][j].setPositionCallback(boost::bind(&ImageView::handlePosition, this, i, j));
      m_handles[i][j].setMoveRequestCallback(boost::bind(&ImageView::handleMoveRequest, this, i, j, _1));
      m_handles[i][j].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));

      m_handleInteractors[i][j].setObject(&m_handles[i][j]);
      m_handleInteractors[i][j].setProximityStatusTip(tip);
      makeLastFollower(m_handleInteractors[i][j]);
    }

    m_lineSegments[i].setPositionCallback(boost::bind(&ImageView::linePosition, this, i));
    m_lineSegments[i].setMoveRequestCallback(boost::bind(&ImageView::lineMoveRequest, this, i, _1));
    m_lineSegments[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));

    m_lineInteractors[i].setObject(&m_lineSegments[i]);
    m_lineInteractors[i].setProximityCursor(Qt::SplitHCursor);
    m_lineInteractors[i].setInteractionCursor(Qt::SplitHCursor);
    m_lineInteractors[i].setProximityStatusTip(tip);
    makeLastFollower(m_lineInteractors[i]);
  }

  // Turn off cutters we don't need anymore.
  for (int i = num_cutters; i < 2; ++i) {
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
  const QRectF virt_rect(virtualDisplayRect());

  switch (m_virtLayout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      painter.setBrush(QColor(0, 0, 255, 50));
      painter.drawRect(virt_rect);

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

  const int num_cutters = m_virtLayout.numCutters();
  for (int i = 0; i < num_cutters; ++i) {
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

QLineF ImageView::widgetCutterLine(const int line_idx) const {
  const QRectF widget_rect(virtualToWidget().mapRect(virtualDisplayRect()));
  QRectF reduced_widget_rect(reducedWidgetArea());
  reduced_widget_rect.setLeft(widget_rect.left());
  reduced_widget_rect.setRight(widget_rect.right());
  // The reason we restore original left and right boundaries is that
  // we want to allow cutter handles to go off-screen horizontally
  // but not vertically.
  QLineF line(customInscribedCutterLine(widgetLayout().cutterLine(line_idx), reduced_widget_rect));

  if (m_handleInteractors[line_idx][1].interactionInProgress(interactionState())) {
    line.setP1(virtualToWidget().map(virtualCutterLine(line_idx).p1()));
  } else if (m_handleInteractors[line_idx][0].interactionInProgress(interactionState())) {
    line.setP2(virtualToWidget().map(virtualCutterLine(line_idx).p2()));
  }

  return line;
}

QLineF ImageView::virtualCutterLine(int line_idx) const {
  const QRectF virt_rect(virtualDisplayRect());

  QRectF widget_rect(virtualToWidget().mapRect(virt_rect));
  const double delta = 0.5 * m_handlePixmap.width();
  widget_rect.adjust(0.0, delta, 0.0, -delta);

  QRectF reduced_virt_rect(widgetToVirtual().mapRect(widget_rect));
  reduced_virt_rect.setLeft(virt_rect.left());
  reduced_virt_rect.setRight(virt_rect.right());
  // The reason we restore original left and right boundaries is that
  // we want to allow cutter handles to go off-screen horizontally
  // but not vertically.
  return customInscribedCutterLine(m_virtLayout.cutterLine(line_idx), reduced_virt_rect);
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
    qreal middle_x = 0.5 * (line.p1().x() + line.p2().x());
    middle_x = qBound(rect.left(), middle_x, rect.right());

    return QLineF(QPointF(middle_x, rect.top()), QPointF(middle_x, rect.bottom()));
  }

  QPointF top_pt;
  QPointF bottom_pt;

  line.intersect(QLineF(rect.topLeft(), rect.topRight()), &top_pt);
  line.intersect(QLineF(rect.bottomLeft(), rect.bottomRight()), &bottom_pt);

  const double top_x = qBound(rect.left(), top_pt.x(), rect.right());
  const double bottom_x = qBound(rect.left(), bottom_pt.x(), rect.right());

  return QLineF(QPointF(top_x, rect.top()), QPointF(bottom_x, rect.bottom()));
}

QPointF ImageView::handlePosition(int line_idx, int handle_idx) const {
  const QLineF line(widgetCutterLine(line_idx));

  return handle_idx == 0 ? line.p1() : line.p2();
}

void ImageView::handleMoveRequest(int line_idx, int handle_idx, const QPointF& pos) {
  const QRectF valid_area(getOccupiedWidgetRect());

  qreal min_x_top = valid_area.left();
  qreal max_x_top = valid_area.right();
  qreal min_x_bottom = valid_area.left();
  qreal max_x_bottom = valid_area.right();

  // preventing intersection with other cutters
  for (int i = 0; i < m_virtLayout.numCutters(); i++) {
    if (i != line_idx) {
      QPointF p_top_i;
      QPointF p_bottom_i;
      QLineF anotherLine = virtualToWidget().map(m_virtLayout.cutterLine(i));
      anotherLine.intersect(QLineF(valid_area.topLeft(), valid_area.topRight()), &p_top_i);
      anotherLine.intersect(QLineF(valid_area.bottomLeft(), valid_area.bottomRight()), &p_bottom_i);

      if ((p_top_i.x() < min_x_top) || (p_top_i.x() > max_x_top) || (p_bottom_i.x() < min_x_bottom)
          || (p_bottom_i.x() > max_x_bottom)) {
        continue;
      }

      if (line_idx > i) {
        min_x_top = p_top_i.x();
        min_x_bottom = p_bottom_i.x();
      } else {
        max_x_top = p_top_i.x();
        max_x_bottom = p_bottom_i.x();
      }
    }
  }

  QLineF virt_line(virtualCutterLine(line_idx));
  if (handle_idx == 0) {
    const QPointF wpt(qBound(min_x_top, pos.x(), max_x_top), valid_area.top());
    virt_line.setP1(widgetToVirtual().map(wpt));
  } else {
    const QPointF wpt(qBound(min_x_bottom, pos.x(), max_x_bottom), valid_area.bottom());
    virt_line.setP2(widgetToVirtual().map(wpt));
  }

  m_virtLayout.setCutterLine(line_idx, virt_line);
  update();
}

QLineF ImageView::linePosition(int line_idx) const {
  return widgetCutterLine(line_idx);
}

void ImageView::lineMoveRequest(int line_idx, QLineF line) {
  // Intersect with top and bottom.
  const QRectF valid_area(getOccupiedWidgetRect());
  QPointF p_top;
  QPointF p_bottom;
  line.intersect(QLineF(valid_area.topLeft(), valid_area.topRight()), &p_top);
  line.intersect(QLineF(valid_area.bottomLeft(), valid_area.bottomRight()), &p_bottom);

  qreal min_x_top = valid_area.left();
  qreal max_x_top = valid_area.right();
  qreal min_x_bottom = valid_area.left();
  qreal max_x_bottom = valid_area.right();

  // preventing intersection with other cutters
  for (int i = 0; i < m_virtLayout.numCutters(); i++) {
    if (i != line_idx) {
      QPointF p_top_i;
      QPointF p_bottom_i;
      QLineF anotherLine = virtualToWidget().map(m_virtLayout.cutterLine(i));
      anotherLine.intersect(QLineF(valid_area.topLeft(), valid_area.topRight()), &p_top_i);
      anotherLine.intersect(QLineF(valid_area.bottomLeft(), valid_area.bottomRight()), &p_bottom_i);

      if ((p_top_i.x() < min_x_top) || (p_top_i.x() > max_x_top) || (p_bottom_i.x() < min_x_bottom)
          || (p_bottom_i.x() > max_x_bottom)) {
        continue;
      }

      if (line_idx > i) {
        min_x_top = p_top_i.x();
        min_x_bottom = p_bottom_i.x();
      } else {
        max_x_top = p_top_i.x();
        max_x_bottom = p_bottom_i.x();
      }
    }
  }

  // Limit movement.
  const double left = qMax(min_x_top - p_top.x(), min_x_bottom - p_bottom.x());
  const double right = qMax(p_top.x() - max_x_top, p_bottom.x() - max_x_bottom);
  if ((left > right) && (left > 0.0)) {
    line.translate(left, 0.0);
  } else if (right > 0.0) {
    line.translate(-right, 0.0);
  }

  m_virtLayout.setCutterLine(line_idx, widgetToVirtual().map(line));
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
  QRectF left_rect(m_virtLayout.leftPageOutline().boundingRect());
  QRectF right_rect(m_virtLayout.rightPageOutline().boundingRect());

  const double x_mid = 0.5 * (left_rect.right() + right_rect.left());
  left_rect.setRight(x_mid);
  right_rect.setLeft(x_mid);

  return virtualToWidget().map(left_rect.center());
}

QPointF ImageView::rightPageCenter() const {
  QRectF left_rect(m_virtLayout.leftPageOutline().boundingRect());
  QRectF right_rect(m_virtLayout.rightPageOutline().boundingRect());

  const double x_mid = 0.5 * (left_rect.right() + right_rect.left());
  left_rect.setRight(x_mid);
  right_rect.setLeft(x_mid);

  return virtualToWidget().map(right_rect.center());
}

void ImageView::unremoveLeftPage() {
  PageInfo page_info(m_ptrPages->unremovePage(PageId(m_imageId, PageId::LEFT_PAGE)));
  m_leftUnremoveButton.unlink();
  m_leftPageRemoved = false;

  update();

  // We need invalidateThumbnail(PageInfo) rather than (PageId),
  // as we are updating page removal status.
  page_info.setId(PageId(m_imageId, PageId::SINGLE_PAGE));
  emit invalidateThumbnail(page_info);
}

void ImageView::unremoveRightPage() {
  PageInfo page_info(m_ptrPages->unremovePage(PageId(m_imageId, PageId::RIGHT_PAGE)));
  m_rightUnremoveButton.unlink();
  m_rightPageRemoved = false;

  update();

  // We need invalidateThumbnail(PageInfo) rather than (PageId),
  // as we are updating page removal status.
  page_info.setId(PageId(m_imageId, PageId::SINGLE_PAGE));
  emit invalidateThumbnail(page_info);
}
}  // namespace page_split