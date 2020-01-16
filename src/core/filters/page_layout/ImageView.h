// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_IMAGEVIEW_H_
#define SCANTAILOR_PAGE_LAYOUT_IMAGEVIEW_H_

#include <imageproc/BinaryImage.h>
#include <interaction/DraggableLineSegment.h>

#include <QMenu>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QTransform>
#include <unordered_map>

#include "Alignment.h"
#include "DragHandler.h"
#include "DraggableObject.h"
#include "Guide.h"
#include "ImageTransformation.h"
#include "ImageViewBase.h"
#include "InteractionHandler.h"
#include "ObjectDragHandler.h"
#include "PageId.h"
#include "ZoomHandler.h"
#include "intrusive_ptr.h"

class Margins;

namespace imageproc {
class GrayImage;
}

namespace page_layout {
class OptionsWidget;
class Settings;

class ImageView : public ImageViewBase, private InteractionHandler {
  Q_OBJECT
 public:
  ImageView(const intrusive_ptr<Settings>& settings,
            const PageId& pageId,
            const QImage& image,
            const QImage& downscaledImage,
            const imageproc::GrayImage& grayImage,
            const ImageTransformation& xform,
            const QRectF& adaptedContentRect,
            const OptionsWidget& optWidget);

  ~ImageView() override;

 signals:

  void invalidateThumbnail(const PageId& pageId);

  void invalidateAllThumbnails();

  void marginsSetLocally(const Margins& marginsMm);

 public slots:

  void marginsSetExternally(const Margins& marginsMm);

  void leftRightLinkToggled(bool linked);

  void topBottomLinkToggled(bool linked);

  void alignmentChanged(const Alignment& alignment);

  void aggregateHardSizeChanged();

 protected:
  void updatePhysSize() override;

 private:
  enum Edge { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };

  enum FitMode { FIT, DONT_FIT };

  enum AggregateSizeChanged { AGGREGATE_SIZE_UNCHANGED, AGGREGATE_SIZE_CHANGED };

  struct StateBeforeResizing {
    /**
     * Transformation from virtual image coordinates to widget coordinates.
     */
    QTransform virtToWidget;

    /**
     * Transformation from widget coordinates to virtual image coordinates.
     */
    QTransform widgetToVirt;

    /**
     * m_middleRect in widget coordinates.
     */
    QRectF middleWidgetRect;

    /**
     * Mouse pointer position in widget coordinates.
     */
    QPointF mousePos;

    /**
     * The point in image that is to be centered on the screen,
     * in pixel image coordinates.
     */
    QPointF focalPoint;
  };

  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) override;

  void onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) override;

  Proximity cornerProximity(int edgeMask, const QRectF* box, const QPointF& mousePos) const;

  Proximity edgeProximity(int edgeMask, const QRectF* box, const QPointF& mousePos) const;

  void dragInitiated(const QPointF& mousePos);

  void innerRectDragContinuation(int edgeMask, const QPointF& mousePos);

  void middleRectDragContinuation(int edgeMask, const QPointF& mousePos);

  void dragFinished();

  void recalcBoxesAndFit(const Margins& marginsMm);

  void updatePresentationTransform(FitMode fitMode);

  void forceNonNegativeHardMargins(QRectF& middleRect) const;

  Margins calcHardMarginsMM() const;

  void recalcOuterRect();

  QSizeF origRectToSizeMM(const QRectF& rect) const;

  AggregateSizeChanged commitHardMargins(const Margins& marginsMm);

  void invalidateThumbnails(AggregateSizeChanged aggSizeChanged);

  void setupContextMenuInteraction();

  void setupGuides();

  void addHorizontalGuide(double y);

  void addVerticalGuide(double x);

  void removeAllGuides();

  void removeGuide(int index);

  QTransform widgetToGuideCs() const;

  QTransform guideToWidgetCs() const;

  void syncGuidesSettings();

  void setupGuideInteraction(int index);

  QLineF guidePosition(int index) const;

  void guideMoveRequest(int index, QLineF line);

  void guideDragFinished();

  QLineF widgetGuideLine(int index) const;

  int getGuideUnderMouse(const QPointF& screenMousePos, const InteractionState& state) const;

  void enableGuidesInteraction(bool state);

  void forceInscribeGuides();

  Proximity rectProximity(const QRectF& box, const QPointF& mousePos) const;

  void innerRectMoveRequest(const QPointF& mousePos, Qt::KeyboardModifiers mask = Qt::NoModifier);

  void buildContentImage(const imageproc::GrayImage& grayImage, const ImageTransformation& xform);

  void attachContentToNearestGuide(const QPointF& pos, Qt::KeyboardModifiers mask = Qt::NoModifier);

  QRect findContentInArea(const QRect& area) const;

  void enableMiddleRectInteraction(bool state);

  bool isShowingMiddleRectEnabled() const;


  DraggableObject m_innerCorners[4];
  ObjectDragHandler m_innerCornerHandlers[4];
  DraggableObject m_innerEdges[4];
  ObjectDragHandler m_innerEdgeHandlers[4];

  DraggableObject m_middleCorners[4];
  ObjectDragHandler m_middleCornerHandlers[4];
  DraggableObject m_middleEdges[4];
  ObjectDragHandler m_middleEdgeHandlers[4];

  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;

  intrusive_ptr<Settings> m_settings;

  const PageId m_pageId;

  /**
   * Transformation between the pixel image coordinates and millimeters,
   * assuming that point (0, 0) in pixel coordinates corresponds to point
   * (0, 0) in millimeter coordinates.
   */
  const QTransform m_pixelsToMmXform;
  const QTransform m_mmToPixelsXform;

  const ImageTransformation m_xform;

  /**
   * Content box in virtual image coordinates.
   */
  const QRectF m_innerRect;

  /**
   * \brief Content box + hard margins in virtual image coordinates.
   *
   * Hard margins are margins that will be there no matter what.
   * Soft margins are those added to extend the page to match its
   * size with other pages.
   */
  QRectF m_middleRect;

  /**
   * \brief Content box + hard + soft margins in virtual image coordinates.
   *
   * Hard margins are margins that will be there no matter what.
   * Soft margins are those added to extend the page to match its
   * size with other pages.
   */
  QRectF m_outerRect;

  /**
   * \brief Aggregate (max width + max height) hard page size.
   *
   * This one is for displaying purposes only.  It changes during
   * dragging, and it may differ from what
   * m_settings->getAggregateHardSizeMM() would return.
   *
   * \see m_committedAggregateHardSizeMM
   */
  QSizeF m_aggregateHardSizeMM;

  /**
   * \brief Aggregate (max width + max height) hard page size.
   *
   * This one is supposed to be the cached version of what
   * m_settings->getAggregateHardSizeMM() would return.
   *
   * \see m_aggregateHardSizeMM
   */
  QSizeF m_committedAggregateHardSizeMM;

  Alignment m_alignment;

  /**
   * Some data saved at the beginning of a resizing operation.
   */
  StateBeforeResizing m_beforeResizing;

  bool m_leftRightLinked;
  bool m_topBottomLinked;

  /** Guides settings. */
  std::unordered_map<int, Guide> m_guides;
  int m_guidesFreeIndex;

  std::unordered_map<int, DraggableLineSegment> m_draggableGuides;
  std::unordered_map<int, ObjectDragHandler> m_draggableGuideHandlers;

  QMenu* m_contextMenu;
  QAction* m_addHorizontalGuideAction;
  QAction* m_addVerticalGuideAction;
  QAction* m_removeAllGuidesAction;
  QAction* m_removeGuideUnderMouseAction;
  QAction* m_guideActionsSeparator;
  QAction* m_showMiddleRectAction;
  QPointF m_lastContextMenuPos;
  int m_guideUnderMouse;

  DraggableObject m_innerRectArea;
  ObjectDragHandler m_innerRectAreaHandler;

  Qt::KeyboardModifier m_innerRectVerticalDragModifier;
  Qt::KeyboardModifier m_innerRectHorizontalDragModifier;

  imageproc::BinaryImage m_contentImage;
  QTransform m_originalToContentImage;
  QTransform m_contentImageToOriginal;

  const bool m_nullContentRect;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_IMAGEVIEW_H_
