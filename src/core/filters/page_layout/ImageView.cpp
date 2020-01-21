// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageView.h"

#include <Despeckle.h>
#include <ImageViewInfoProvider.h>
#include <NullTaskStatus.h>
#include <PolygonUtils.h>
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

using namespace imageproc;

namespace page_layout {
ImageView::ImageView(const intrusive_ptr<Settings>& settings,
                     const PageId& pageId,
                     const QImage& image,
                     const QImage& downscaledImage,
                     const imageproc::GrayImage& grayImage,
                     const ImageTransformation& xform,
                     const QRectF& adaptedContentRect,
                     const OptionsWidget& optWidget)
    : ImageViewBase(image,
                    downscaledImage,
                    ImagePresentation(xform.transform(), xform.resultingPreCropArea()),
                    Margins(5, 5, 5, 5)),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_settings(settings),
      m_pageId(pageId),
      m_pixelsToMmXform(UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES)),
      m_mmToPixelsXform(m_pixelsToMmXform.inverted()),
      m_xform(xform),
      m_innerRect(adaptedContentRect),
      m_aggregateHardSizeMM(settings->getAggregateHardSizeMM()),
      m_committedAggregateHardSizeMM(m_aggregateHardSizeMM),
      m_alignment(optWidget.alignment()),
      m_leftRightLinked(optWidget.leftRightLinked()),
      m_topBottomLinked(optWidget.topBottomLinked()),
      m_guidesFreeIndex(0),
      m_contextMenu(new QMenu(this)),
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

    Qt::CursorShape cornerCursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
    m_innerCornerHandlers[i].setProximityCursor(cornerCursor);
    m_innerCornerHandlers[i].setInteractionCursor(cornerCursor);
    m_middleCornerHandlers[i].setProximityCursor(cornerCursor);
    m_middleCornerHandlers[i].setInteractionCursor(cornerCursor);

    Qt::CursorShape edgeCursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
    m_innerEdgeHandlers[i].setProximityCursor(edgeCursor);
    m_innerEdgeHandlers[i].setInteractionCursor(edgeCursor);
    m_middleEdgeHandlers[i].setProximityCursor(edgeCursor);
    m_middleEdgeHandlers[i].setInteractionCursor(edgeCursor);

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

  recalcBoxesAndFit(optWidget.marginsMM());

  buildContentImage(grayImage, xform);
}

ImageView::~ImageView() = default;

void ImageView::marginsSetExternally(const Margins& marginsMm) {
  const AggregateSizeChanged changed = commitHardMargins(marginsMm);

  recalcBoxesAndFit(marginsMm);

  invalidateThumbnails(changed);
}

void ImageView::leftRightLinkToggled(const bool linked) {
  m_leftRightLinked = linked;
  if (linked) {
    Margins marginsMm(calcHardMarginsMM());
    if (marginsMm.left() != marginsMm.right()) {
      const double newMargin = std::min(marginsMm.left(), marginsMm.right());
      marginsMm.setLeft(newMargin);
      marginsMm.setRight(newMargin);

      const AggregateSizeChanged changed = commitHardMargins(marginsMm);

      recalcBoxesAndFit(marginsMm);
      emit marginsSetLocally(marginsMm);

      invalidateThumbnails(changed);
    }
  }
}

void ImageView::topBottomLinkToggled(const bool linked) {
  m_topBottomLinked = linked;
  if (linked) {
    Margins marginsMm(calcHardMarginsMM());
    if (marginsMm.top() != marginsMm.bottom()) {
      const double newMargin = std::min(marginsMm.top(), marginsMm.bottom());
      marginsMm.setTop(newMargin);
      marginsMm.setBottom(newMargin);

      const AggregateSizeChanged changed = commitHardMargins(marginsMm);

      recalcBoxesAndFit(marginsMm);
      emit marginsSetLocally(marginsMm);

      invalidateThumbnails(changed);
    }
  }
}

void ImageView::alignmentChanged(const Alignment& alignment) {
  m_alignment = alignment;

  const Settings::AggregateSizeChanged sizeChanged = m_settings->setPageAlignment(m_pageId, alignment);

  recalcBoxesAndFit(calcHardMarginsMM());

  enableGuidesInteraction(!m_alignment.isNull());
  forceInscribeGuides();

  enableMiddleRectInteraction(isShowingMiddleRectEnabled());

  if (sizeChanged == Settings::AGGREGATE_SIZE_CHANGED) {
    emit invalidateAllThumbnails();
  } else {
    emit invalidateThumbnail(m_pageId);
  }
}

void ImageView::aggregateHardSizeChanged() {
  m_aggregateHardSizeMM = m_settings->getAggregateHardSizeMM();
  m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;
  recalcOuterRect();
  updatePresentationTransform(FIT);
}

void ImageView::onPaint(QPainter& painter, const InteractionState& interaction) {
  QColor bgColor;
  QColor fgColor;
  if (m_alignment.isNull()) {
    // "Align with other pages" is turned off.
    // Different color is useful on a thumbnail list to
    // distinguish "safe" pages from potentially problematic ones.
    bgColor = QColor(0x58, 0x7f, 0xf4, 70);
    fgColor = QColor(0x00, 0x52, 0xff);
  } else {
    bgColor = QColor(0xbb, 0x00, 0xff, 40);
    fgColor = QColor(0xbe, 0x5b, 0xec);
  }

  QPainterPath outerOutline;
  outerOutline.addPolygon(PolygonUtils::round(m_alignment.isNull() ? m_middleRect : m_outerRect));

  QPainterPath contentOutline;
  contentOutline.addPolygon(PolygonUtils::round(m_innerRect));

  painter.setRenderHint(QPainter::Antialiasing, false);

  painter.setPen(Qt::NoPen);
  painter.setBrush(bgColor);

  if (!m_nullContentRect) {
    painter.drawPath(outerOutline.subtracted(contentOutline));
  } else {
    painter.drawPath(outerOutline);
  }

  QPen pen(fgColor);
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

Proximity ImageView::cornerProximity(const int edgeMask, const QRectF* box, const QPointF& mousePos) const {
  const QRectF r(virtualToWidget().mapRect(*box));
  QPointF pt;

  if (edgeMask & TOP) {
    pt.setY(r.top());
  } else if (edgeMask & BOTTOM) {
    pt.setY(r.bottom());
  }

  if (edgeMask & LEFT) {
    pt.setX(r.left());
  } else if (edgeMask & RIGHT) {
    pt.setX(r.right());
  }
  return Proximity(pt, mousePos);
}

Proximity ImageView::edgeProximity(const int edgeMask, const QRectF* box, const QPointF& mousePos) const {
  const QRectF r(virtualToWidget().mapRect(*box));
  QLineF line;

  switch (edgeMask) {
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
  return Proximity::pointAndLineSegment(mousePos, line);
}

void ImageView::dragInitiated(const QPointF& mousePos) {
  m_beforeResizing.middleWidgetRect = virtualToWidget().mapRect(m_middleRect);
  m_beforeResizing.virtToWidget = virtualToWidget();
  m_beforeResizing.widgetToVirt = widgetToVirtual();
  m_beforeResizing.mousePos = mousePos;
  m_beforeResizing.focalPoint = getWidgetFocalPoint();
}

void ImageView::innerRectDragContinuation(int edgeMask, const QPointF& mousePos) {
  // What really happens when we resize the inner box is resizing
  // the middle box in the opposite direction and moving the scene
  // on screen so that the object being dragged is still under mouse.

  const QPointF delta(mousePos - m_beforeResizing.mousePos);
  qreal leftAdjust = 0;
  qreal rightAdjust = 0;
  qreal topAdjust = 0;
  qreal bottomAdjust = 0;

  if (edgeMask & LEFT) {
    leftAdjust = delta.x();
    if (m_leftRightLinked) {
      rightAdjust = -leftAdjust;
    }
  } else if (edgeMask & RIGHT) {
    rightAdjust = delta.x();
    if (m_leftRightLinked) {
      leftAdjust = -rightAdjust;
    }
  }
  if (edgeMask & TOP) {
    topAdjust = delta.y();
    if (m_topBottomLinked) {
      bottomAdjust = -topAdjust;
    }
  } else if (edgeMask & BOTTOM) {
    bottomAdjust = delta.y();
    if (m_topBottomLinked) {
      topAdjust = -bottomAdjust;
    }
  }

  QRectF widgetRect(m_beforeResizing.middleWidgetRect);
  widgetRect.adjust(-leftAdjust, -topAdjust, -rightAdjust, -bottomAdjust);

  m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widgetRect);
  forceNonNegativeHardMargins(m_middleRect);
  widgetRect = m_beforeResizing.virtToWidget.mapRect(m_middleRect);

  qreal effectiveDx = 0;
  qreal effectiveDy = 0;

  const QRectF& oldWidgetRect = m_beforeResizing.middleWidgetRect;
  if (edgeMask & LEFT) {
    effectiveDx = oldWidgetRect.left() - widgetRect.left();
  } else if (edgeMask & RIGHT) {
    effectiveDx = oldWidgetRect.right() - widgetRect.right();
  }
  if (edgeMask & TOP) {
    effectiveDy = oldWidgetRect.top() - widgetRect.top();
  } else if (edgeMask & BOTTOM) {
    effectiveDy = oldWidgetRect.bottom() - widgetRect.bottom();
  }

  // Updating the focal point is what makes the image move
  // as we drag an inner edge.
  QPointF fp(m_beforeResizing.focalPoint);
  fp += QPointF(effectiveDx, effectiveDy);
  setWidgetFocalPoint(fp);

  m_aggregateHardSizeMM = m_settings->getAggregateHardSizeMM(m_pageId, origRectToSizeMM(m_middleRect), m_alignment);

  recalcOuterRect();

  updatePresentationTransform(DONT_FIT);

  emit marginsSetLocally(calcHardMarginsMM());
}  // ImageView::innerRectDragContinuation

void ImageView::middleRectDragContinuation(const int edgeMask, const QPointF& mousePos) {
  const QPointF delta(mousePos - m_beforeResizing.mousePos);
  qreal leftAdjust = 0;
  qreal rightAdjust = 0;
  qreal topAdjust = 0;
  qreal bottomAdjust = 0;

  const QRectF bounds(maxViewportRect());
  const QRectF oldMiddleRect(m_beforeResizing.middleWidgetRect);

  if (edgeMask & LEFT) {
    leftAdjust = delta.x();
    if (oldMiddleRect.left() + leftAdjust < bounds.left()) {
      leftAdjust = bounds.left() - oldMiddleRect.left();
    }
    if (m_leftRightLinked) {
      rightAdjust = -leftAdjust;
    }
  } else if (edgeMask & RIGHT) {
    rightAdjust = delta.x();
    if (oldMiddleRect.right() + rightAdjust > bounds.right()) {
      rightAdjust = bounds.right() - oldMiddleRect.right();
    }
    if (m_leftRightLinked) {
      leftAdjust = -rightAdjust;
    }
  }
  if (edgeMask & TOP) {
    topAdjust = delta.y();
    if (oldMiddleRect.top() + topAdjust < bounds.top()) {
      topAdjust = bounds.top() - oldMiddleRect.top();
    }
    if (m_topBottomLinked) {
      bottomAdjust = -topAdjust;
    }
  } else if (edgeMask & BOTTOM) {
    bottomAdjust = delta.y();
    if (oldMiddleRect.bottom() + bottomAdjust > bounds.bottom()) {
      bottomAdjust = bounds.bottom() - oldMiddleRect.bottom();
    }
    if (m_topBottomLinked) {
      topAdjust = -bottomAdjust;
    }
  }

  {
    QRectF widgetRect(oldMiddleRect);
    widgetRect.adjust(leftAdjust, topAdjust, rightAdjust, bottomAdjust);

    m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widgetRect);
    forceNonNegativeHardMargins(m_middleRect);  // invalidates widgetRect
  }

  m_aggregateHardSizeMM = m_settings->getAggregateHardSizeMM(m_pageId, origRectToSizeMM(m_middleRect), m_alignment);

  recalcOuterRect();

  updatePresentationTransform(DONT_FIT);

  emit marginsSetLocally(calcHardMarginsMM());
}  // ImageView::middleRectDragContinuation

void ImageView::dragFinished() {
  const AggregateSizeChanged aggSizeChanged(commitHardMargins(calcHardMarginsMM()));

  const QRectF extendedViewport(maxViewportRect().adjusted(-0.5, -0.5, 0.5, 0.5));
  if (extendedViewport.contains(m_beforeResizing.middleWidgetRect)) {
    updatePresentationTransform(FIT);
  } else {
    updatePresentationTransform(DONT_FIT);
  }

  invalidateThumbnails(aggSizeChanged);
}

/**
 * Updates m_middleRect and m_outerRect based on \p marginsMm,
 * m_aggregateHardSizeMM and m_alignment, updates the displayed area.
 */
void ImageView::recalcBoxesAndFit(const Margins& marginsMm) {
  const QTransform virtToMm(virtualToImage() * m_pixelsToMmXform);
  const QTransform mmToVirt(m_mmToPixelsXform * imageToVirtual());

  QPolygonF polyMm(virtToMm.map(m_innerRect));
  Utils::extendPolyRectWithMargins(polyMm, marginsMm);

  const QRectF middleRect(mmToVirt.map(polyMm).boundingRect());

  const QSizeF hardSizeMm(QLineF(polyMm[0], polyMm[1]).length(), QLineF(polyMm[0], polyMm[3]).length());
  const Margins softMarginsMm(Utils::calcSoftMarginsMM(
      hardSizeMm, m_aggregateHardSizeMM, m_alignment, m_settings->getPageParams(m_pageId)->contentRect(),
      m_settings->getPageParams(m_pageId)->contentSizeMM(), m_settings->getPageParams(m_pageId)->pageRect()));

  Utils::extendPolyRectWithMargins(polyMm, softMarginsMm);

  const QRectF outerRect(mmToVirt.map(polyMm).boundingRect());
  updateTransformAndFixFocalPoint(
      ImagePresentation(imageToVirtual(), m_xform.resultingPreCropArea().intersected(outerRect), outerRect),
      CENTER_IF_FITS);

  m_middleRect = middleRect;
  m_outerRect = outerRect;

  updatePhysSize();

  forceInscribeGuides();
}

/**
 * Updates the virtual image area to be displayed by ImageViewBase,
 * optionally ensuring that this area completely fits into the view.
 *
 * \note virtualToImage() and imageToVirtual() are not affected by this.
 */
void ImageView::updatePresentationTransform(const FitMode fitMode) {
  if (fitMode == DONT_FIT) {
    updateTransformPreservingScale(
        ImagePresentation(imageToVirtual(), m_xform.resultingPreCropArea().intersected(m_outerRect), m_outerRect));
  } else {
    setZoomLevel(1.0);
    updateTransformAndFixFocalPoint(
        ImagePresentation(imageToVirtual(), m_xform.resultingPreCropArea().intersected(m_outerRect), m_outerRect),
        CENTER_IF_FITS);
  }
}

void ImageView::forceNonNegativeHardMargins(QRectF& middleRect) const {
  if (middleRect.left() > m_innerRect.left()) {
    middleRect.setLeft(m_innerRect.left());
  }
  if (middleRect.right() < m_innerRect.right()) {
    middleRect.setRight(m_innerRect.right());
  }
  if (middleRect.top() > m_innerRect.top()) {
    middleRect.setTop(m_innerRect.top());
  }
  if (middleRect.bottom() < m_innerRect.bottom()) {
    middleRect.setBottom(m_innerRect.bottom());
  }
}

/**
 * \brief Calculates margins in millimeters between m_innerRect and m_middleRect.
 */
Margins ImageView::calcHardMarginsMM() const {
  const QPointF center(m_innerRect.center());

  const QLineF topMarginLine(QPointF(center.x(), m_middleRect.top()), QPointF(center.x(), m_innerRect.top()));

  const QLineF bottomMarginLine(QPointF(center.x(), m_innerRect.bottom()), QPointF(center.x(), m_middleRect.bottom()));

  const QLineF leftMarginLine(QPointF(m_middleRect.left(), center.y()), QPointF(m_innerRect.left(), center.y()));

  const QLineF rightMarginLine(QPointF(m_innerRect.right(), center.y()), QPointF(m_middleRect.right(), center.y()));

  const QTransform virtToMm(virtualToImage() * m_pixelsToMmXform);

  Margins margins;
  margins.setTop(virtToMm.map(topMarginLine).length());
  margins.setBottom(virtToMm.map(bottomMarginLine).length());
  margins.setLeft(virtToMm.map(leftMarginLine).length());
  margins.setRight(virtToMm.map(rightMarginLine).length());
  return margins;
}  // ImageView::calcHardMarginsMM

/**
 * \brief Recalculates m_outerRect based on m_middleRect, m_aggregateHardSizeMM
 *        and m_alignment.
 */
void ImageView::recalcOuterRect() {
  const QTransform virtToMm(virtualToImage() * m_pixelsToMmXform);
  const QTransform mmToVirt(m_mmToPixelsXform * imageToVirtual());

  QPolygonF polyMm(virtToMm.map(m_middleRect));

  const QSizeF hardSizeMm(QLineF(polyMm[0], polyMm[1]).length(), QLineF(polyMm[0], polyMm[3]).length());
  const Margins softMarginsMm(Utils::calcSoftMarginsMM(
      hardSizeMm, m_aggregateHardSizeMM, m_alignment, m_settings->getPageParams(m_pageId)->contentRect(),
      m_settings->getPageParams(m_pageId)->contentSizeMM(), m_settings->getPageParams(m_pageId)->pageRect()));

  Utils::extendPolyRectWithMargins(polyMm, softMarginsMm);

  m_outerRect = mmToVirt.map(polyMm).boundingRect();
  updatePhysSize();

  forceInscribeGuides();
}

QSizeF ImageView::origRectToSizeMM(const QRectF& rect) const {
  const QTransform virtToMm(virtualToImage() * m_pixelsToMmXform);

  const QLineF horLine(rect.topLeft(), rect.topRight());
  const QLineF vertLine(rect.topLeft(), rect.bottomLeft());

  const QSizeF sizeMm(virtToMm.map(horLine).length(), virtToMm.map(vertLine).length());
  return sizeMm;
}

ImageView::AggregateSizeChanged ImageView::commitHardMargins(const Margins& marginsMm) {
  m_settings->setHardMarginsMM(m_pageId, marginsMm);
  m_aggregateHardSizeMM = m_settings->getAggregateHardSizeMM();

  AggregateSizeChanged changed = AGGREGATE_SIZE_UNCHANGED;
  if (m_committedAggregateHardSizeMM != m_aggregateHardSizeMM) {
    changed = AGGREGATE_SIZE_CHANGED;
  }

  m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;
  return changed;
}

void ImageView::invalidateThumbnails(const AggregateSizeChanged aggSizeChanged) {
  if (aggSizeChanged == AGGREGATE_SIZE_CHANGED) {
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
  m_showMiddleRectAction->setChecked(m_settings->isShowingMiddleRectEnabled());

  connect(m_addHorizontalGuideAction, &QAction::triggered,
          [this]() { addHorizontalGuide(widgetToGuideCs().map(m_lastContextMenuPos).y()); });
  connect(m_addVerticalGuideAction, &QAction::triggered,
          [this]() { addVerticalGuide(widgetToGuideCs().map(m_lastContextMenuPos).x()); });
  connect(m_removeAllGuidesAction, &QAction::triggered, boost::bind(&ImageView::removeAllGuides, this));
  connect(m_removeGuideUnderMouseAction, &QAction::triggered, [this]() { removeGuide(m_guideUnderMouse); });
  connect(m_showMiddleRectAction, &QAction::toggled, [this](bool checked) {
    if (!m_alignment.isNull() && !m_nullContentRect) {
      enableMiddleRectInteraction(checked);
      m_settings->enableShowingMiddleRect(checked);
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
  for (const Guide& guide : m_settings->guides()) {
    m_guides[m_guidesFreeIndex] = m_mmToPixelsXform.map(guide);
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
  m_settings->guides().clear();
  for (const auto& idxAndGuide : m_guides) {
    m_settings->guides().emplace_back(m_pixelsToMmXform.map(idxAndGuide.second));
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
  const QRectF validArea(virtualToWidget().mapRect(m_outerRect));

  // Limit movement.
  if (m_guides[index].getOrientation() == Qt::Horizontal) {
    const double linePos = line.y1();
    const double top = validArea.top() - linePos;
    const double bottom = linePos - validArea.bottom();
    if (top > 0.0) {
      line.translate(0.0, top);
    } else if (bottom > 0.0) {
      line.translate(0.0, -bottom);
    }
  } else {
    const double linePos = line.x1();
    const double left = validArea.left() - linePos;
    const double right = linePos - validArea.right();
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
  const QRectF widgetRect(viewport()->rect());
  const Guide& guide = m_guides.at(index);
  QLineF guideLine = guideToWidgetCs().map(guide);
  if (guide.getOrientation() == Qt::Horizontal) {
    guideLine = QLineF(widgetRect.left(), guideLine.y1(), widgetRect.right(), guideLine.y2());
  } else {
    guideLine = QLineF(guideLine.x1(), widgetRect.top(), guideLine.x2(), widgetRect.bottom());
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

  bool needSync = false;
  for (const auto& idxAndGuide : m_guides) {
    const Guide& guide = idxAndGuide.second;
    const double pos = guide.getPosition();
    if (guide.getOrientation() == Qt::Vertical) {
      if (std::abs(pos) > (m_outerRect.width() / 2)) {
        m_guides[idxAndGuide.first].setPosition(std::copysign(m_outerRect.width() / 2, pos));
        needSync = true;
      }
    } else {
      if (std::abs(pos) > (m_outerRect.height() / 2)) {
        m_guides[idxAndGuide.first].setPosition(std::copysign(m_outerRect.height() / 2, pos));
        needSync = true;
      }
    }
  }

  if (needSync) {
    syncGuidesSettings();
  }
}

void ImageView::innerRectMoveRequest(const QPointF& mousePos, const Qt::KeyboardModifiers mask) {
  QPointF delta(mousePos - m_beforeResizing.mousePos);
  if (mask == m_innerRectVerticalDragModifier) {
    delta.setX(0);
  } else if (mask == m_innerRectHorizontalDragModifier) {
    delta.setY(0);
  }

  QRectF widgetRect(m_beforeResizing.middleWidgetRect);
  widgetRect.translate(-delta);
  m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widgetRect);
  forceNonNegativeHardMargins(m_middleRect);

  // Updating the focal point is what makes the image move
  // as we drag an inner edge.
  QPointF fp(m_beforeResizing.focalPoint);
  fp += delta;
  setWidgetFocalPoint(fp);

  m_aggregateHardSizeMM = m_settings->getAggregateHardSizeMM(m_pageId, origRectToSizeMM(m_middleRect), m_alignment);

  recalcOuterRect();

  updatePresentationTransform(DONT_FIT);

  emit marginsSetLocally(calcHardMarginsMM());
}

Proximity ImageView::rectProximity(const QRectF& box, const QPointF& mousePos) const {
  double value = virtualToWidget().mapRect(box).contains(mousePos) ? 0 : std::numeric_limits<double>::max();
  return Proximity::fromSqDist(value);
}

void ImageView::buildContentImage(const GrayImage& grayImage, const ImageTransformation& xform) {
  ImageTransformation xform150dpi(xform);
  xform150dpi.preScaleToDpi(Dpi(150, 150));

  if (xform150dpi.resultingRect().toRect().isEmpty()) {
    return;
  }

  QImage gray150(transformToGray(grayImage, xform150dpi.transform(), xform150dpi.resultingRect().toRect(),
                                 OutsidePixels::assumeColor(Qt::white)));

  m_contentImage = binarizeWolf(gray150, QSize(51, 51), 50);

  PolygonRasterizer::fillExcept(m_contentImage, WHITE, xform150dpi.resultingPreCropArea(), Qt::WindingFill);

  Despeckle::despeckleInPlace(m_contentImage, Dpi(150, 150), Despeckle::NORMAL, NullTaskStatus());

  m_originalToContentImage = xform150dpi.transform();
  m_contentImageToOriginal = m_originalToContentImage.inverted();
}

QRect ImageView::findContentInArea(const QRect& area) const {
  const uint32_t* imageLine = m_contentImage.data();
  const int imageStride = m_contentImage.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  int top = std::numeric_limits<int>::max();
  int left = std::numeric_limits<int>::max();
  int bottom = std::numeric_limits<int>::min();
  int right = std::numeric_limits<int>::min();

  imageLine += area.top() * imageStride;
  for (int y = area.top(); y <= area.bottom(); ++y) {
    for (int x = area.left(); x <= area.right(); ++x) {
      if (imageLine[x >> 5] & (msb >> (x & 31))) {
        top = std::min(top, y);
        left = std::min(left, x);
        bottom = std::max(bottom, y);
        right = std::max(right, x);
      }
    }
    imageLine += imageStride;
  }

  if (top > bottom) {
    return QRect();
  }

  QRect foundArea = QRect(left, top, right - left + 1, bottom - top + 1);
  foundArea.adjust(-1, -1, 1, 1);
  return foundArea;
}

void ImageView::onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() == Qt::LeftButton) {
    if (!m_alignment.isNull() && !m_guides.empty()) {
      attachContentToNearestGuide(QPointF(0.5, 0.5) + event->pos(), event->modifiers());
    }
  }
}

void ImageView::attachContentToNearestGuide(const QPointF& pos, const Qt::KeyboardModifiers mask) {
  const QTransform widgetToContentImage(widgetToImage() * m_originalToContentImage);
  const QTransform contentImageToVirtual(m_contentImageToOriginal * imageToVirtual());

  const QPointF contentPos = widgetToContentImage.map(pos);

  QRect findingArea((contentPos - QPointF(15, 15)).toPoint(), QSize(30, 30));
  findingArea = findingArea.intersected(m_contentImage.rect());
  if (findingArea.isEmpty()) {
    return;
  }

  QRect foundArea = findContentInArea(findingArea);
  if (foundArea.isEmpty()) {
    return;
  }

  const QRectF foundAreaInVirtual = contentImageToVirtual.mapRect(QRectF(foundArea)).intersected(m_innerRect);
  if (foundAreaInVirtual.isEmpty()) {
    return;
  }

  QPointF delta;
  {
    const bool onlyHorizontalDirection = (mask == m_innerRectHorizontalDragModifier);
    const bool onlyVerticalDirection = (mask == m_innerRectVerticalDragModifier);
    const bool bothDirections = (mask == (m_innerRectVerticalDragModifier | m_innerRectHorizontalDragModifier));

    double minDistX = std::numeric_limits<int>::max();
    double minDistY = std::numeric_limits<int>::max();
    for (const auto& idxAndGuide : m_guides) {
      const Guide& guide = idxAndGuide.second;
      if (guide.getOrientation() == Qt::Vertical) {
        if (onlyVerticalDirection) {
          continue;
        }

        const double guidePosInVirtual = guide.getPosition() + m_outerRect.center().x();
        const double diffLeft = guidePosInVirtual - foundAreaInVirtual.left();
        const double diffRight = guidePosInVirtual - foundAreaInVirtual.right();
        const double diff = std::abs(diffLeft) <= std::abs(diffRight) ? diffLeft : diffRight;
        const double dist = std::abs(diff);
        const double minDist = (bothDirections) ? minDistX : std::min(minDistX, minDistY);
        if (dist < minDist) {
          minDistX = dist;
          delta.setX(diff);
          if (!bothDirections) {
            delta.setY(0.0);
          }
        }
      } else {
        if (onlyHorizontalDirection) {
          continue;
        }

        const double guidePosInVirtual = guide.getPosition() + m_outerRect.center().y();
        const double diffTop = guidePosInVirtual - foundAreaInVirtual.top();
        const double diffBottom = guidePosInVirtual - foundAreaInVirtual.bottom();
        const double diff = std::abs(diffTop) <= std::abs(diffBottom) ? diffTop : diffBottom;
        const double dist = std::abs(diff);
        const double minDist = (bothDirections) ? minDistY : std::min(minDistX, minDistY);
        if (dist < minDist) {
          minDistY = dist;
          delta.setY(diff);
          if (!bothDirections) {
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
    QRectF correctedMiddleRect = m_middleRect;
    correctedMiddleRect.translate(-delta);
    correctedMiddleRect |= m_innerRect;

    {
      // Correct the delta in case of the middle rect size changed.
      // It means that the center is shifted resulting
      // the guide will change its position, so we need an extra addition to delta.
      const double xCorrection
          = std::copysign(std::max(0.0, correctedMiddleRect.width() - m_middleRect.width()), delta.x());
      const double yCorrection
          = std::copysign(std::max(0.0, correctedMiddleRect.height() - m_middleRect.height()), delta.y());
      delta.setX(delta.x() + xCorrection);
      delta.setY(delta.y() + yCorrection);

      correctedMiddleRect.translate(-xCorrection, -yCorrection);
      correctedMiddleRect |= m_innerRect;
    }

    {
      // Restrict the delta value in order not to be out of the outer rect.
      const double xCorrection
          = std::copysign(std::max(0.0, correctedMiddleRect.width() - m_outerRect.width()), delta.x());
      const double yCorrection
          = std::copysign(std::max(0.0, correctedMiddleRect.height() - m_outerRect.height()), delta.y());
      delta.setX(delta.x() - xCorrection);
      delta.setY(delta.y() - yCorrection);
    }
  }

  // Move the page content.
  dragInitiated(virtualToWidget().map(QPointF(0, 0)));
  innerRectMoveRequest(virtualToWidget().map(delta));
  dragFinished();
}

void ImageView::enableMiddleRectInteraction(const bool state) {
  bool internalState = m_middleCornerHandlers[0].is_linked();
  if (state == internalState) {
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
  }

  update();
}

bool ImageView::isShowingMiddleRectEnabled() const {
  return (!m_nullContentRect && m_settings->isShowingMiddleRectEnabled()) || m_alignment.isNull();
}
}  // namespace page_layout