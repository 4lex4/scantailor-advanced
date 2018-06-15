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
#include <ImageViewInfoProvider.h>
#include <UnitsConverter.h>
#include <imageproc/Binarize.h>
#include <imageproc/GrayImage.h>
#include <imageproc/PolygonRasterizer.h>
#include <imageproc/Transform.h>
#include <QMouseEvent>
#include <QPainter>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "ImagePresentation.h"
#include "OptionsWidget.h"
#include "Params.h"
#include "Settings.h"
#include "Utils.h"
#include "imageproc/PolygonUtils.h"

using namespace imageproc;

namespace page_layout {
ImageView::ImageView(const intrusive_ptr<Settings>& settings,
                     const PageId& page_id,
                     const QImage& image,
                     const QImage& downscaled_image,
                     const imageproc::GrayImage& gray_image,
                     const ImageTransformation& xform,
                     const QRectF& adapted_content_rect,
                     const OptionsWidget& opt_widget)
    : ImageViewBase(image,
                    downscaled_image,
                    ImagePresentation(xform.transform(), xform.resultingPreCropArea()),
                    Margins(5, 5, 5, 5)),
      m_xform(xform),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_ptrSettings(settings),
      m_pageId(page_id),
      m_pixelsToMmXform(UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES)),
      m_mmToPixelsXform(m_pixelsToMmXform.inverted()),
      m_innerRect(adapted_content_rect),
      m_aggregateHardSizeMM(settings->getAggregateHardSizeMM()),
      m_committedAggregateHardSizeMM(m_aggregateHardSizeMM),
      m_alignment(opt_widget.alignment()),
      m_leftRightLinked(opt_widget.leftRightLinked()),
      m_topBottomLinked(opt_widget.topBottomLinked()),
      m_contextMenu(new QMenu(this)),
      m_guidesFreeIndex(0),
      m_guideUnderMouse(-1),
      m_innerRectVerticalDragModifier(Qt::ControlModifier),
      m_innerRectHorizontalDragModifier(Qt::ShiftModifier),
      m_nullContentRect((m_innerRect.width() < 1) && (m_innerRect.height() < 1)) {
  setMouseTracking(true);

  interactionState().setDefaultStatusTip(tr("Resize margins by dragging any of the solid lines."));

  // Setup interaction stuff.
  static const int masks_by_edge[] = {TOP, RIGHT, BOTTOM, LEFT};
  static const int masks_by_corner[] = {TOP | LEFT, TOP | RIGHT, BOTTOM | RIGHT, BOTTOM | LEFT};
  for (int i = 0; i < 4; ++i) {
    // Proximity priority - inner rect higher than middle, corners higher than edges.
    m_innerCorners[i].setProximityPriorityCallback(boost::lambda::constant(4));
    m_innerEdges[i].setProximityPriorityCallback(boost::lambda::constant(3));
    m_middleCorners[i].setProximityPriorityCallback(boost::lambda::constant(2));
    m_middleEdges[i].setProximityPriorityCallback(boost::lambda::constant(1));

    // Proximity.
    m_innerCorners[i].setProximityCallback(
        boost::bind(&ImageView::cornerProximity, this, masks_by_corner[i], &m_innerRect, _1));
    m_middleCorners[i].setProximityCallback(
        boost::bind(&ImageView::cornerProximity, this, masks_by_corner[i], &m_middleRect, _1));
    m_innerEdges[i].setProximityCallback(
        boost::bind(&ImageView::edgeProximity, this, masks_by_edge[i], &m_innerRect, _1));
    m_middleEdges[i].setProximityCallback(
        boost::bind(&ImageView::edgeProximity, this, masks_by_edge[i], &m_middleRect, _1));
    // Drag initiation.
    m_innerCorners[i].setDragInitiatedCallback(boost::bind(&ImageView::dragInitiated, this, _1));
    m_middleCorners[i].setDragInitiatedCallback(boost::bind(&ImageView::dragInitiated, this, _1));
    m_innerEdges[i].setDragInitiatedCallback(boost::bind(&ImageView::dragInitiated, this, _1));
    m_middleEdges[i].setDragInitiatedCallback(boost::bind(&ImageView::dragInitiated, this, _1));

    // Drag continuation.
    m_innerCorners[i].setDragContinuationCallback(
        boost::bind(&ImageView::innerRectDragContinuation, this, masks_by_corner[i], _1));
    m_middleCorners[i].setDragContinuationCallback(
        boost::bind(&ImageView::middleRectDragContinuation, this, masks_by_corner[i], _1));
    m_innerEdges[i].setDragContinuationCallback(
        boost::bind(&ImageView::innerRectDragContinuation, this, masks_by_edge[i], _1));
    m_middleEdges[i].setDragContinuationCallback(
        boost::bind(&ImageView::middleRectDragContinuation, this, masks_by_edge[i], _1));
    // Drag finishing.
    m_innerCorners[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));
    m_middleCorners[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));
    m_innerEdges[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));
    m_middleEdges[i].setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));

    m_innerCornerHandlers[i].setObject(&m_innerCorners[i]);
    m_middleCornerHandlers[i].setObject(&m_middleCorners[i]);
    m_innerEdgeHandlers[i].setObject(&m_innerEdges[i]);
    m_middleEdgeHandlers[i].setObject(&m_middleEdges[i]);

    Qt::CursorShape corner_cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
    m_innerCornerHandlers[i].setProximityCursor(corner_cursor);
    m_innerCornerHandlers[i].setInteractionCursor(corner_cursor);
    m_middleCornerHandlers[i].setProximityCursor(corner_cursor);
    m_middleCornerHandlers[i].setInteractionCursor(corner_cursor);

    Qt::CursorShape edge_cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
    m_innerEdgeHandlers[i].setProximityCursor(edge_cursor);
    m_innerEdgeHandlers[i].setInteractionCursor(edge_cursor);
    m_middleEdgeHandlers[i].setProximityCursor(edge_cursor);
    m_middleEdgeHandlers[i].setInteractionCursor(edge_cursor);

    if (isShowingMiddleRectEnabled()) {
      makeLastFollower(m_middleCornerHandlers[i]);
      makeLastFollower(m_middleEdgeHandlers[i]);
    }
    if (!m_nullContentRect) {
      makeLastFollower(m_innerCornerHandlers[i]);
      makeLastFollower(m_innerEdgeHandlers[i]);
    }
  }

  {
    m_innerRectArea.setProximityCallback(boost::bind(&ImageView::rectProximity, this, boost::ref(m_innerRect), _1));
    m_innerRectArea.setDragInitiatedCallback(boost::bind(&ImageView::dragInitiated, this, _1));
    m_innerRectArea.setDragContinuationCallback(boost::bind(&ImageView::innerRectMoveRequest, this, _1, _2));
    m_innerRectArea.setDragFinishedCallback(boost::bind(&ImageView::dragFinished, this));
    m_innerRectAreaHandler.setObject(&m_innerRectArea);
    m_innerRectAreaHandler.setProximityStatusTip(tr("Hold left mouse button to drag the page content."));
    m_innerRectAreaHandler.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));
    m_innerRectAreaHandler.setKeyboardModifiers({m_innerRectVerticalDragModifier, m_innerRectHorizontalDragModifier,
                                                 m_innerRectVerticalDragModifier | m_innerRectHorizontalDragModifier});
    Qt::CursorShape cursor = Qt::DragMoveCursor;
    m_innerRectAreaHandler.setProximityCursor(cursor);
    m_innerRectAreaHandler.setInteractionCursor(cursor);
    makeLastFollower(m_innerRectAreaHandler);
  }

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  setupContextMenuInteraction();
  setupGuides();

  recalcBoxesAndFit(opt_widget.marginsMM());

  buildContentImage(gray_image, xform);
}

ImageView::~ImageView() = default;

void ImageView::marginsSetExternally(const Margins& margins_mm) {
  const AggregateSizeChanged changed = commitHardMargins(margins_mm);

  recalcBoxesAndFit(margins_mm);

  invalidateThumbnails(changed);
}

void ImageView::leftRightLinkToggled(const bool linked) {
  m_leftRightLinked = linked;
  if (linked) {
    Margins margins_mm(calcHardMarginsMM());
    if (margins_mm.left() != margins_mm.right()) {
      const double new_margin = std::min(margins_mm.left(), margins_mm.right());
      margins_mm.setLeft(new_margin);
      margins_mm.setRight(new_margin);

      const AggregateSizeChanged changed = commitHardMargins(margins_mm);

      recalcBoxesAndFit(margins_mm);
      emit marginsSetLocally(margins_mm);

      invalidateThumbnails(changed);
    }
  }
}

void ImageView::topBottomLinkToggled(const bool linked) {
  m_topBottomLinked = linked;
  if (linked) {
    Margins margins_mm(calcHardMarginsMM());
    if (margins_mm.top() != margins_mm.bottom()) {
      const double new_margin = std::min(margins_mm.top(), margins_mm.bottom());
      margins_mm.setTop(new_margin);
      margins_mm.setBottom(new_margin);

      const AggregateSizeChanged changed = commitHardMargins(margins_mm);

      recalcBoxesAndFit(margins_mm);
      emit marginsSetLocally(margins_mm);

      invalidateThumbnails(changed);
    }
  }
}

void ImageView::alignmentChanged(const Alignment& alignment) {
  m_alignment = alignment;

  const Settings::AggregateSizeChanged size_changed = m_ptrSettings->setPageAlignment(m_pageId, alignment);

  recalcBoxesAndFit(calcHardMarginsMM());

  enableGuidesInteraction(!m_alignment.isNull());
  forceInscribeGuides();

  enableMiddleRectInteraction(isShowingMiddleRectEnabled());

  if (size_changed == Settings::AGGREGATE_SIZE_CHANGED) {
    emit invalidateAllThumbnails();
  } else {
    emit invalidateThumbnail(m_pageId);
  }
}

void ImageView::aggregateHardSizeChanged() {
  m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM();
  m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;
  recalcOuterRect();
  updatePresentationTransform(FIT);
}

void ImageView::onPaint(QPainter& painter, const InteractionState& interaction) {
  QColor bg_color;
  QColor fg_color;
  if (m_alignment.isNull()) {
    // "Align with other pages" is turned off.
    // Different color is useful on a thumbnail list to
    // distinguish "safe" pages from potentially problematic ones.
    bg_color = QColor(0x58, 0x7f, 0xf4, 70);
    fg_color = QColor(0x00, 0x52, 0xff);
  } else {
    bg_color = QColor(0xbb, 0x00, 0xff, 40);
    fg_color = QColor(0xbe, 0x5b, 0xec);
  }

  QPainterPath outer_outline;
  outer_outline.addPolygon(PolygonUtils::round(m_alignment.isNull() ? m_middleRect : m_outerRect));

  QPainterPath content_outline;
  content_outline.addPolygon(PolygonUtils::round(m_innerRect));

  painter.setRenderHint(QPainter::Antialiasing, false);

  painter.setPen(Qt::NoPen);
  painter.setBrush(bg_color);

  if (!m_nullContentRect) {
    painter.drawPath(outer_outline.subtracted(content_outline));
  } else {
    painter.drawPath(outer_outline);
  }

  QPen pen(fg_color);
  pen.setCosmetic(true);
  pen.setWidthF(2.0);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);

  if (isShowingMiddleRectEnabled()) {
    painter.drawRect(m_middleRect);
  }
  if (!m_nullContentRect) {
    painter.drawRect(m_innerRect);
  }

  if (!m_alignment.isNull()) {
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    painter.drawRect(m_outerRect);

    // Draw guides.
    if (!m_guides.empty()) {
      painter.setWorldTransform(QTransform());

      QPen pen(QColor(0x00, 0x9d, 0x9f));
      pen.setStyle(Qt::DashLine);
      pen.setCosmetic(true);
      pen.setWidthF(2.0);
      painter.setPen(pen);
      painter.setBrush(Qt::NoBrush);

      for (const auto& idxAndGuide : m_guides) {
        const QLineF guide(widgetGuideLine(idxAndGuide.first));
        painter.drawLine(guide);
      }
    }
  }
}  // ImageView::onPaint

Proximity ImageView::cornerProximity(const int edge_mask, const QRectF* box, const QPointF& mouse_pos) const {
  const QRectF r(virtualToWidget().mapRect(*box));
  QPointF pt;

  if (edge_mask & TOP) {
    pt.setY(r.top());
  } else if (edge_mask & BOTTOM) {
    pt.setY(r.bottom());
  }

  if (edge_mask & LEFT) {
    pt.setX(r.left());
  } else if (edge_mask & RIGHT) {
    pt.setX(r.right());
  }

  return Proximity(pt, mouse_pos);
}

Proximity ImageView::edgeProximity(const int edge_mask, const QRectF* box, const QPointF& mouse_pos) const {
  const QRectF r(virtualToWidget().mapRect(*box));
  QLineF line;

  switch (edge_mask) {
    case TOP:
      line.setP1(r.topLeft());
      line.setP2(r.topRight());
      break;
    case BOTTOM:
      line.setP1(r.bottomLeft());
      line.setP2(r.bottomRight());
      break;
    case LEFT:
      line.setP1(r.topLeft());
      line.setP2(r.bottomLeft());
      break;
    case RIGHT:
      line.setP1(r.topRight());
      line.setP2(r.bottomRight());
      break;
    default:
      assert(!"Unreachable");
  }

  return Proximity::pointAndLineSegment(mouse_pos, line);
}

void ImageView::dragInitiated(const QPointF& mouse_pos) {
  m_beforeResizing.middleWidgetRect = virtualToWidget().mapRect(m_middleRect);
  m_beforeResizing.virtToWidget = virtualToWidget();
  m_beforeResizing.widgetToVirt = widgetToVirtual();
  m_beforeResizing.mousePos = mouse_pos;
  m_beforeResizing.focalPoint = getWidgetFocalPoint();
}

void ImageView::innerRectDragContinuation(int edge_mask, const QPointF& mouse_pos) {
  // What really happens when we resize the inner box is resizing
  // the middle box in the opposite direction and moving the scene
  // on screen so that the object being dragged is still under mouse.

  const QPointF delta(mouse_pos - m_beforeResizing.mousePos);
  qreal left_adjust = 0;
  qreal right_adjust = 0;
  qreal top_adjust = 0;
  qreal bottom_adjust = 0;

  if (edge_mask & LEFT) {
    left_adjust = delta.x();
    if (m_leftRightLinked) {
      right_adjust = -left_adjust;
    }
  } else if (edge_mask & RIGHT) {
    right_adjust = delta.x();
    if (m_leftRightLinked) {
      left_adjust = -right_adjust;
    }
  }
  if (edge_mask & TOP) {
    top_adjust = delta.y();
    if (m_topBottomLinked) {
      bottom_adjust = -top_adjust;
    }
  } else if (edge_mask & BOTTOM) {
    bottom_adjust = delta.y();
    if (m_topBottomLinked) {
      top_adjust = -bottom_adjust;
    }
  }

  QRectF widget_rect(m_beforeResizing.middleWidgetRect);
  widget_rect.adjust(-left_adjust, -top_adjust, -right_adjust, -bottom_adjust);

  m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widget_rect);
  forceNonNegativeHardMargins(m_middleRect);
  widget_rect = m_beforeResizing.virtToWidget.mapRect(m_middleRect);

  qreal effective_dx = 0;
  qreal effective_dy = 0;

  const QRectF& old_widget_rect = m_beforeResizing.middleWidgetRect;
  if (edge_mask & LEFT) {
    effective_dx = old_widget_rect.left() - widget_rect.left();
  } else if (edge_mask & RIGHT) {
    effective_dx = old_widget_rect.right() - widget_rect.right();
  }
  if (edge_mask & TOP) {
    effective_dy = old_widget_rect.top() - widget_rect.top();
  } else if (edge_mask & BOTTOM) {
    effective_dy = old_widget_rect.bottom() - widget_rect.bottom();
  }

  // Updating the focal point is what makes the image move
  // as we drag an inner edge.
  QPointF fp(m_beforeResizing.focalPoint);
  fp += QPointF(effective_dx, effective_dy);
  setWidgetFocalPoint(fp);

  m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(m_pageId, origRectToSizeMM(m_middleRect), m_alignment);

  recalcOuterRect();

  updatePresentationTransform(DONT_FIT);

  emit marginsSetLocally(calcHardMarginsMM());
}  // ImageView::innerRectDragContinuation

void ImageView::middleRectDragContinuation(const int edge_mask, const QPointF& mouse_pos) {
  const QPointF delta(mouse_pos - m_beforeResizing.mousePos);
  qreal left_adjust = 0;
  qreal right_adjust = 0;
  qreal top_adjust = 0;
  qreal bottom_adjust = 0;

  const QRectF bounds(maxViewportRect());
  const QRectF old_middle_rect(m_beforeResizing.middleWidgetRect);

  if (edge_mask & LEFT) {
    left_adjust = delta.x();
    if (old_middle_rect.left() + left_adjust < bounds.left()) {
      left_adjust = bounds.left() - old_middle_rect.left();
    }
    if (m_leftRightLinked) {
      right_adjust = -left_adjust;
    }
  } else if (edge_mask & RIGHT) {
    right_adjust = delta.x();
    if (old_middle_rect.right() + right_adjust > bounds.right()) {
      right_adjust = bounds.right() - old_middle_rect.right();
    }
    if (m_leftRightLinked) {
      left_adjust = -right_adjust;
    }
  }
  if (edge_mask & TOP) {
    top_adjust = delta.y();
    if (old_middle_rect.top() + top_adjust < bounds.top()) {
      top_adjust = bounds.top() - old_middle_rect.top();
    }
    if (m_topBottomLinked) {
      bottom_adjust = -top_adjust;
    }
  } else if (edge_mask & BOTTOM) {
    bottom_adjust = delta.y();
    if (old_middle_rect.bottom() + bottom_adjust > bounds.bottom()) {
      bottom_adjust = bounds.bottom() - old_middle_rect.bottom();
    }
    if (m_topBottomLinked) {
      top_adjust = -bottom_adjust;
    }
  }

  {
    QRectF widget_rect(old_middle_rect);
    widget_rect.adjust(left_adjust, top_adjust, right_adjust, bottom_adjust);

    m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widget_rect);
    forceNonNegativeHardMargins(m_middleRect);  // invalidates widget_rect
  }

  m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(m_pageId, origRectToSizeMM(m_middleRect), m_alignment);

  recalcOuterRect();

  updatePresentationTransform(DONT_FIT);

  emit marginsSetLocally(calcHardMarginsMM());
}  // ImageView::middleRectDragContinuation

void ImageView::dragFinished() {
  const AggregateSizeChanged agg_size_changed(commitHardMargins(calcHardMarginsMM()));

  const QRectF extended_viewport(maxViewportRect().adjusted(-0.5, -0.5, 0.5, 0.5));
  if (extended_viewport.contains(m_beforeResizing.middleWidgetRect)) {
    updatePresentationTransform(FIT);
  } else {
    updatePresentationTransform(DONT_FIT);
  }

  invalidateThumbnails(agg_size_changed);
}

/**
 * Updates m_middleRect and m_outerRect based on \p margins_mm,
 * m_aggregateHardSizeMM and m_alignment, updates the displayed area.
 */
void ImageView::recalcBoxesAndFit(const Margins& margins_mm) {
  const QTransform virt_to_mm(virtualToImage() * m_pixelsToMmXform);
  const QTransform mm_to_virt(m_mmToPixelsXform * imageToVirtual());

  QPolygonF poly_mm(virt_to_mm.map(m_innerRect));
  Utils::extendPolyRectWithMargins(poly_mm, margins_mm);

  const QRectF middle_rect(mm_to_virt.map(poly_mm).boundingRect());

  const QSizeF hard_size_mm(QLineF(poly_mm[0], poly_mm[1]).length(), QLineF(poly_mm[0], poly_mm[3]).length());
  const Margins soft_margins_mm(Utils::calcSoftMarginsMM(
      hard_size_mm, m_aggregateHardSizeMM, m_alignment, m_ptrSettings->getPageParams(m_pageId)->contentRect(),
      m_ptrSettings->getPageParams(m_pageId)->contentSizeMM(), m_ptrSettings->getAggregateContentRect(),
      m_ptrSettings->getPageParams(m_pageId)->pageRect()));

  Utils::extendPolyRectWithMargins(poly_mm, soft_margins_mm);

  const QRectF outer_rect(mm_to_virt.map(poly_mm).boundingRect());
  updateTransformAndFixFocalPoint(
      ImagePresentation(imageToVirtual(), m_xform.resultingPreCropArea().intersected(outer_rect), outer_rect),
      CENTER_IF_FITS);

  m_middleRect = middle_rect;
  m_outerRect = outer_rect;

  updatePhysSize();

  forceInscribeGuides();
}

/**
 * Updates the virtual image area to be displayed by ImageViewBase,
 * optionally ensuring that this area completely fits into the view.
 *
 * \note virtualToImage() and imageToVirtual() are not affected by this.
 */
void ImageView::updatePresentationTransform(const FitMode fit_mode) {
  if (fit_mode == DONT_FIT) {
    updateTransformPreservingScale(
        ImagePresentation(imageToVirtual(), m_xform.resultingPreCropArea().intersected(m_outerRect), m_outerRect));
  } else {
    setZoomLevel(1.0);
    updateTransformAndFixFocalPoint(
        ImagePresentation(imageToVirtual(), m_xform.resultingPreCropArea().intersected(m_outerRect), m_outerRect),
        CENTER_IF_FITS);
  }
}

void ImageView::forceNonNegativeHardMargins(QRectF& middle_rect) const {
  if (middle_rect.left() > m_innerRect.left()) {
    middle_rect.setLeft(m_innerRect.left());
  }
  if (middle_rect.right() < m_innerRect.right()) {
    middle_rect.setRight(m_innerRect.right());
  }
  if (middle_rect.top() > m_innerRect.top()) {
    middle_rect.setTop(m_innerRect.top());
  }
  if (middle_rect.bottom() < m_innerRect.bottom()) {
    middle_rect.setBottom(m_innerRect.bottom());
  }
}

/**
 * \brief Calculates margins in millimeters between m_innerRect and m_middleRect.
 */
Margins ImageView::calcHardMarginsMM() const {
  const QPointF center(m_innerRect.center());

  const QLineF top_margin_line(QPointF(center.x(), m_middleRect.top()), QPointF(center.x(), m_innerRect.top()));

  const QLineF bottom_margin_line(QPointF(center.x(), m_innerRect.bottom()),
                                  QPointF(center.x(), m_middleRect.bottom()));

  const QLineF left_margin_line(QPointF(m_middleRect.left(), center.y()), QPointF(m_innerRect.left(), center.y()));

  const QLineF right_margin_line(QPointF(m_innerRect.right(), center.y()), QPointF(m_middleRect.right(), center.y()));

  const QTransform virt_to_mm(virtualToImage() * m_pixelsToMmXform);

  Margins margins;
  margins.setTop(virt_to_mm.map(top_margin_line).length());
  margins.setBottom(virt_to_mm.map(bottom_margin_line).length());
  margins.setLeft(virt_to_mm.map(left_margin_line).length());
  margins.setRight(virt_to_mm.map(right_margin_line).length());

  return margins;
}  // ImageView::calcHardMarginsMM

/**
 * \brief Recalculates m_outerRect based on m_middleRect, m_aggregateHardSizeMM
 *        and m_alignment.
 */
void ImageView::recalcOuterRect() {
  const QTransform virt_to_mm(virtualToImage() * m_pixelsToMmXform);
  const QTransform mm_to_virt(m_mmToPixelsXform * imageToVirtual());

  QPolygonF poly_mm(virt_to_mm.map(m_middleRect));

  const QSizeF hard_size_mm(QLineF(poly_mm[0], poly_mm[1]).length(), QLineF(poly_mm[0], poly_mm[3]).length());
  const Margins soft_margins_mm(Utils::calcSoftMarginsMM(
      hard_size_mm, m_aggregateHardSizeMM, m_alignment, m_ptrSettings->getPageParams(m_pageId)->contentRect(),
      m_ptrSettings->getPageParams(m_pageId)->contentSizeMM(), m_ptrSettings->getAggregateContentRect(),
      m_ptrSettings->getPageParams(m_pageId)->pageRect()));

  Utils::extendPolyRectWithMargins(poly_mm, soft_margins_mm);

  m_outerRect = mm_to_virt.map(poly_mm).boundingRect();
  updatePhysSize();

  forceInscribeGuides();
}

QSizeF ImageView::origRectToSizeMM(const QRectF& rect) const {
  const QTransform virt_to_mm(virtualToImage() * m_pixelsToMmXform);

  const QLineF hor_line(rect.topLeft(), rect.topRight());
  const QLineF vert_line(rect.topLeft(), rect.bottomLeft());

  const QSizeF size_mm(virt_to_mm.map(hor_line).length(), virt_to_mm.map(vert_line).length());

  return size_mm;
}

ImageView::AggregateSizeChanged ImageView::commitHardMargins(const Margins& margins_mm) {
  m_ptrSettings->setHardMarginsMM(m_pageId, margins_mm);
  m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM();

  AggregateSizeChanged changed = AGGREGATE_SIZE_UNCHANGED;
  if (m_committedAggregateHardSizeMM != m_aggregateHardSizeMM) {
    changed = AGGREGATE_SIZE_CHANGED;
  }

  m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;

  return changed;
}

void ImageView::invalidateThumbnails(const AggregateSizeChanged agg_size_changed) {
  if (agg_size_changed == AGGREGATE_SIZE_CHANGED) {
    emit invalidateAllThumbnails();
  } else {
    emit invalidateThumbnail(m_pageId);
  }
}

void ImageView::updatePhysSize() {
  if (m_outerRect.isValid()) {
    infoProvider().setPhysSize(m_outerRect.size());
  } else {
    ImageViewBase::updatePhysSize();
  }
}

void ImageView::setupContextMenuInteraction() {
  m_addHorizontalGuideAction = m_contextMenu->addAction(tr("Add a horizontal guide"));
  m_addVerticalGuideAction = m_contextMenu->addAction(tr("Add a vertical guide"));
  m_removeAllGuidesAction = m_contextMenu->addAction(tr("Remove all the guides"));
  m_removeGuideUnderMouseAction = m_contextMenu->addAction(tr("Remove this guide"));
  m_guideActionsSeparator = m_contextMenu->addSeparator();
  m_showMiddleRectAction = m_contextMenu->addAction(tr("Show hard margins rectangle"));
  m_showMiddleRectAction->setCheckable(true);
  m_showMiddleRectAction->setChecked(m_ptrSettings->isShowingMiddleRectEnabled());

  connect(m_addHorizontalGuideAction, &QAction::triggered,
          [this]() { addHorizontalGuide(widgetToGuideCs().map(m_lastContextMenuPos).y()); });
  connect(m_addVerticalGuideAction, &QAction::triggered,
          [this]() { addVerticalGuide(widgetToGuideCs().map(m_lastContextMenuPos).x()); });
  connect(m_removeAllGuidesAction, &QAction::triggered, boost::bind(&ImageView::removeAllGuides, this));
  connect(m_removeGuideUnderMouseAction, &QAction::triggered, [this]() { removeGuide(m_guideUnderMouse); });
  connect(m_showMiddleRectAction, &QAction::toggled, [this](bool checked) {
    if (!m_alignment.isNull() && !m_nullContentRect) {
      enableMiddleRectInteraction(checked);
      m_ptrSettings->enableShowingMiddleRect(checked);
    }
  });
}

void ImageView::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {
  if (interaction.captured()) {
    // No context menus during resizing.
    return;
  }
  if (m_alignment.isNull()) {
    return;
  }

  const QPointF eventPos = QPointF(0.5, 0.5) + event->pos();
  // No context menus outside the outer rect.
  if (!m_outerRect.contains(widgetToVirtual().map(eventPos))) {
    return;
  }

  m_guideUnderMouse = getGuideUnderMouse(eventPos, interaction);
  if (m_guideUnderMouse == -1) {
    m_addHorizontalGuideAction->setVisible(true);
    m_addVerticalGuideAction->setVisible(true);
    m_removeAllGuidesAction->setVisible(!m_guides.empty());
    m_removeGuideUnderMouseAction->setVisible(false);
    m_guideActionsSeparator->setVisible(!m_nullContentRect);
    m_showMiddleRectAction->setVisible(!m_nullContentRect);

    m_lastContextMenuPos = eventPos;
  } else {
    m_addHorizontalGuideAction->setVisible(false);
    m_addVerticalGuideAction->setVisible(false);
    m_removeAllGuidesAction->setVisible(false);
    m_removeGuideUnderMouseAction->setVisible(true);
    m_guideActionsSeparator->setVisible(false);
    m_showMiddleRectAction->setVisible(false);
  }

  m_contextMenu->popup(event->globalPos());
}

void ImageView::setupGuides() {
  for (const Guide& guide : m_ptrSettings->guides()) {
    m_guides[m_guidesFreeIndex] = guide;
    setupGuideInteraction(m_guidesFreeIndex++);
  }
}

void ImageView::addHorizontalGuide(double y) {
  if (m_alignment.isNull()) {
    return;
  }

  m_guides[m_guidesFreeIndex] = Guide(Qt::Horizontal, y);
  setupGuideInteraction(m_guidesFreeIndex++);

  update();
  syncGuidesSettings();
}

void ImageView::addVerticalGuide(double x) {
  if (m_alignment.isNull()) {
    return;
  }

  m_guides[m_guidesFreeIndex] = Guide(Qt::Vertical, x);
  setupGuideInteraction(m_guidesFreeIndex++);

  update();
  syncGuidesSettings();
}

void ImageView::removeAllGuides() {
  if (m_alignment.isNull()) {
    return;
  }

  m_draggableGuideHandlers.clear();
  m_draggableGuides.clear();

  m_guides.clear();
  m_guidesFreeIndex = 0;

  update();
  syncGuidesSettings();
}

void ImageView::removeGuide(const int index) {
  if (m_alignment.isNull()) {
    return;
  }
  if (m_guides.find(index) == m_guides.end()) {
    return;
  }

  m_draggableGuideHandlers.erase(index);
  m_draggableGuides.erase(index);

  m_guides.erase(index);

  update();
  syncGuidesSettings();
}

QTransform ImageView::widgetToGuideCs() const {
  QTransform xform(widgetToVirtual());
  xform *= QTransform().translate(-m_outerRect.center().x(), -m_outerRect.center().y());

  return xform;
}

QTransform ImageView::guideToWidgetCs() const {
  return widgetToGuideCs().inverted();
}

void ImageView::syncGuidesSettings() {
  m_ptrSettings->guides().clear();
  for (const auto& idxAndGuide : m_guides) {
    m_ptrSettings->guides().push_back(idxAndGuide.second);
  }
}

void ImageView::setupGuideInteraction(const int index) {
  m_draggableGuides[index].setProximityPriority(1);
  m_draggableGuides[index].setPositionCallback(boost::bind(&ImageView::guidePosition, this, index));
  m_draggableGuides[index].setMoveRequestCallback(boost::bind(&ImageView::guideMoveRequest, this, index, _1));
  m_draggableGuides[index].setDragFinishedCallback(boost::bind(&ImageView::guideDragFinished, this));

  const Qt::CursorShape cursorShape
      = (m_guides[index].getOrientation() == Qt::Horizontal) ? Qt::SplitVCursor : Qt::SplitHCursor;
  m_draggableGuideHandlers[index].setObject(&m_draggableGuides[index]);
  m_draggableGuideHandlers[index].setProximityCursor(cursorShape);
  m_draggableGuideHandlers[index].setInteractionCursor(cursorShape);
  m_draggableGuideHandlers[index].setProximityStatusTip(tr("Drag the guide."));
  m_draggableGuideHandlers[index].setKeyboardModifiers({Qt::ShiftModifier});

  if (!m_alignment.isNull()) {
    makeLastFollower(m_draggableGuideHandlers[index]);
  }
}

QLineF ImageView::guidePosition(const int index) const {
  return widgetGuideLine(index);
}

void ImageView::guideMoveRequest(const int index, QLineF line) {
  const QRectF valid_area(virtualToWidget().mapRect(m_outerRect));

  // Limit movement.
  if (m_guides[index].getOrientation() == Qt::Horizontal) {
    const double linePos = line.y1();
    const double top = valid_area.top() - linePos;
    const double bottom = linePos - valid_area.bottom();
    if (top > 0.0) {
      line.translate(0.0, top);
    } else if (bottom > 0.0) {
      line.translate(0.0, -bottom);
    }
  } else {
    const double linePos = line.x1();
    const double left = valid_area.left() - linePos;
    const double right = linePos - valid_area.right();
    if (left > 0.0) {
      line.translate(left, 0.0);
    } else if (right > 0.0) {
      line.translate(-right, 0.0);
    }
  }

  m_guides[index] = widgetToGuideCs().map(line);
  update();
}

void ImageView::guideDragFinished() {
  syncGuidesSettings();
}

QLineF ImageView::widgetGuideLine(const int index) const {
  const QRectF widget_rect(viewport()->rect());
  const Guide& guide = m_guides.at(index);
  QLineF guideLine = guideToWidgetCs().map(guide);
  if (guide.getOrientation() == Qt::Horizontal) {
    guideLine = QLineF(widget_rect.left(), guideLine.y1(), widget_rect.right(), guideLine.y2());
  } else {
    guideLine = QLineF(guideLine.x1(), widget_rect.top(), guideLine.x2(), widget_rect.bottom());
  }

  return guideLine;
}

int ImageView::getGuideUnderMouse(const QPointF& screenMousePos, const InteractionState& state) const {
  for (const auto& idxAndGuide : m_guides) {
    const QLineF guide(widgetGuideLine(idxAndGuide.first));
    if (Proximity::pointAndLineSegment(screenMousePos, guide) <= state.proximityThreshold()) {
      return idxAndGuide.first;
    }
  }

  return -1;
}

void ImageView::enableGuidesInteraction(const bool state) {
  if (state) {
    for (auto& idxAndGuideHandler : m_draggableGuideHandlers) {
      makeLastFollower(idxAndGuideHandler.second);
    }
  } else {
    for (auto& idxAndGuideHandler : m_draggableGuideHandlers) {
      idxAndGuideHandler.second.unlink();
    }
  }
}

void ImageView::forceInscribeGuides() {
  if (m_alignment.isNull()) {
    return;
  }

  bool need_sync = false;
  for (const auto& idxAndGuide : m_guides) {
    const Guide& guide = idxAndGuide.second;
    const double pos = guide.getPosition();
    if (guide.getOrientation() == Qt::Vertical) {
      if (std::abs(pos) > (m_outerRect.width() / 2)) {
        m_guides[idxAndGuide.first].setPosition(std::copysign(m_outerRect.width() / 2, pos));
        need_sync = true;
      }
    } else {
      if (std::abs(pos) > (m_outerRect.height() / 2)) {
        m_guides[idxAndGuide.first].setPosition(std::copysign(m_outerRect.height() / 2, pos));
        need_sync = true;
      }
    }
  }

  if (need_sync) {
    syncGuidesSettings();
  }
}

void ImageView::innerRectMoveRequest(const QPointF& mouse_pos, const Qt::KeyboardModifiers mask) {
  QPointF delta(mouse_pos - m_beforeResizing.mousePos);
  if (mask == m_innerRectVerticalDragModifier) {
    delta.setX(0);
  } else if (mask == m_innerRectHorizontalDragModifier) {
    delta.setY(0);
  }

  QRectF widget_rect(m_beforeResizing.middleWidgetRect);
  widget_rect.translate(-delta);
  m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widget_rect);
  forceNonNegativeHardMargins(m_middleRect);

  // Updating the focal point is what makes the image move
  // as we drag an inner edge.
  QPointF fp(m_beforeResizing.focalPoint);
  fp += delta;
  setWidgetFocalPoint(fp);

  m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(m_pageId, origRectToSizeMM(m_middleRect), m_alignment);

  recalcOuterRect();

  updatePresentationTransform(DONT_FIT);

  emit marginsSetLocally(calcHardMarginsMM());
}

Proximity ImageView::rectProximity(const QRectF& box, const QPointF& mouse_pos) const {
  double value = virtualToWidget().mapRect(box).contains(mouse_pos) ? 0 : std::numeric_limits<double>::max();
  return Proximity::fromSqDist(value);
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

void ImageView::onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() == Qt::LeftButton) {
    if (!m_alignment.isNull() && !m_guides.empty()) {
      attachContentToNearestGuide(QPointF(0.5, 0.5) + event->pos(), event->modifiers());
    }
  }
}

void ImageView::attachContentToNearestGuide(const QPointF& pos, const Qt::KeyboardModifiers mask) {
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

  const QRectF found_area_in_virtual = content_image_to_virtual.mapRect(QRectF(found_area)).intersected(m_innerRect);
  if (found_area_in_virtual.isEmpty()) {
    return;
  }

  QPointF delta;
  {
    const bool only_horizontal_direction = (mask == m_innerRectHorizontalDragModifier);
    const bool only_vertical_direction = (mask == m_innerRectVerticalDragModifier);
    const bool both_directions = (mask == (m_innerRectVerticalDragModifier | m_innerRectHorizontalDragModifier));

    double min_dist_x = std::numeric_limits<int>::max();
    double min_dist_y = std::numeric_limits<int>::max();
    for (const auto& idxAndGuide : m_guides) {
      const Guide& guide = idxAndGuide.second;
      if (guide.getOrientation() == Qt::Vertical) {
        if (only_vertical_direction) {
          continue;
        }

        const double guide_pos_in_virtual = guide.getPosition() + m_outerRect.center().x();
        const double diff_left = guide_pos_in_virtual - found_area_in_virtual.left();
        const double diff_right = guide_pos_in_virtual - found_area_in_virtual.right();
        const double diff = std::abs(diff_left) <= std::abs(diff_right) ? diff_left : diff_right;
        const double dist = std::abs(diff);
        const double min_dist = (both_directions) ? min_dist_x : std::min(min_dist_x, min_dist_y);
        if (dist < min_dist) {
          min_dist_x = dist;
          delta.setX(diff);
          if (!both_directions) {
            delta.setY(0.0);
          }
        }
      } else {
        if (only_horizontal_direction) {
          continue;
        }

        const double guide_pos_in_virtual = guide.getPosition() + m_outerRect.center().y();
        const double diff_top = guide_pos_in_virtual - found_area_in_virtual.top();
        const double diff_bottom = guide_pos_in_virtual - found_area_in_virtual.bottom();
        const double diff = std::abs(diff_top) <= std::abs(diff_bottom) ? diff_top : diff_bottom;
        const double dist = std::abs(diff);
        const double min_dist = (both_directions) ? min_dist_y : std::min(min_dist_x, min_dist_y);
        if (dist < min_dist) {
          min_dist_y = dist;
          delta.setY(diff);
          if (!both_directions) {
            delta.setX(0.0);
          }
        }
      }
    }
  }
  if (delta.isNull()) {
    return;
  }

  {
    QRectF corrected_middle_rect = m_middleRect;
    corrected_middle_rect.translate(-delta);
    corrected_middle_rect |= m_innerRect;

    {
      // Correct the delta in case of the middle rect size changed.
      // It means that the center is shifted resulting
      // the guide will change its position, so we need an extra addition to delta.
      const double x_correction
          = std::copysign(std::max(0.0, corrected_middle_rect.width() - m_middleRect.width()), delta.x());
      const double y_correction
          = std::copysign(std::max(0.0, corrected_middle_rect.height() - m_middleRect.height()), delta.y());
      delta.setX(delta.x() + x_correction);
      delta.setY(delta.y() + y_correction);

      corrected_middle_rect.translate(-x_correction, -y_correction);
      corrected_middle_rect |= m_innerRect;
    }

    {
      // Restrict the delta value in order not to be out of the outer rect.
      const double x_correction
          = std::copysign(std::max(0.0, corrected_middle_rect.width() - m_outerRect.width()), delta.x());
      const double y_correction
          = std::copysign(std::max(0.0, corrected_middle_rect.height() - m_outerRect.height()), delta.y());
      delta.setX(delta.x() - x_correction);
      delta.setY(delta.y() - y_correction);
    }
  }

  // Move the page content.
  dragInitiated(virtualToWidget().map(QPointF(0, 0)));
  innerRectMoveRequest(virtualToWidget().map(delta));
  dragFinished();
}

void ImageView::enableMiddleRectInteraction(const bool state) {
  bool internal_state = m_middleCornerHandlers[0].is_linked();
  if (state == internal_state) {
    // Don't enable or disable the interaction if that's already done.
    return;
  }

  if (state) {
    for (int i = 0; i < 4; ++i) {
      makeLastFollower(m_middleCornerHandlers[i]);
      makeLastFollower(m_middleEdgeHandlers[i]);
    }
  } else {
    for (int i = 0; i < 4; ++i) {
      m_middleCornerHandlers[i].unlink();
      m_middleEdgeHandlers[i].unlink();
    }
  };

  update();
}

bool ImageView::isShowingMiddleRectEnabled() const {
  return (!m_nullContentRect && m_ptrSettings->isShowingMiddleRectEnabled()) || m_alignment.isNull();
}
}  // namespace page_layout