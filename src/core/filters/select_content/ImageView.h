// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_IMAGEVIEW_H_
#define SCANTAILOR_SELECT_CONTENT_IMAGEVIEW_H_

#include <core/ContentMask.h>
#include <interaction/DraggablePolygon.h>

#include <QRectF>
#include <QSizeF>
#include <QString>

#include "DragHandler.h"
#include "DraggableLineSegment.h"
#include "DraggablePoint.h"
#include "ImageViewBase.h"
#include "ObjectDragHandler.h"
#include "ZoomHandler.h"

class ImageTransformation;
class QMenu;

namespace select_content {
class ImageView : public ImageViewBase, private InteractionHandler {
  Q_OBJECT
 public:
  /**
   * \p contentRect is in virtual image coordinates.
   */
  ImageView(const QImage& image,
            const QImage& downscaledImage,
            const ContentMask& contentMask,
            const ImageTransformation& xform,
            const QRectF& contentRect,
            const QRectF& pageRect,
            bool pageRectEnabled);

  ~ImageView() override;

 signals:

  void manualContentRectSet(const QRectF& contentRect);

  void manualPageRectSet(const QRectF& pageRect);

  void pageRectSizeChanged(const QSizeF& size);

 public slots:

  void pageRectSetExternally(const QRectF& pageRect);

  void setPageRectEnabled(bool state);

 private slots:

  void createContentBox();

  void removeContentBox();

 private:
  enum Edge { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };

  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) override;

  void onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) override;

  QPointF contentRectCornerPosition(int edgeMask) const;

  void contentRectCornerMoveRequest(int edgeMask, const QPointF& pos);

  QLineF contentRectEdgePosition(int edge) const;

  void contentRectEdgeMoveRequest(int edge, const QLineF& line);

  void contentRectDragFinished();

  QPointF pageRectCornerPosition(int edgeMask) const;

  void pageRectCornerMoveRequest(int edgeMask, const QPointF& pos);

  QLineF pageRectEdgePosition(int edge) const;

  void pageRectEdgeMoveRequest(int edge, const QLineF& line);

  void pageRectDragFinished();

  void forceInsideImage(QRectF& widgetRect, int edgeMask) const;

  void forcePageRectDescribeContent();

  QRectF contentRectPosition() const;

  void contentRectMoveRequest(const QPolygonF& polyMoved);

  QRectF pageRectPosition() const;

  void pageRectMoveRequest(const QPolygonF& polyMoved);

  void correctContentBox(const QPointF& pos, Qt::KeyboardModifiers mask = Qt::NoModifier);

  void enableContentRectInteraction(bool state);

  void enablePageRectInteraction(bool state);


  DraggablePoint m_contentRectCorners[4];
  ObjectDragHandler m_contentRectCornerHandlers[4];

  DraggableLineSegment m_contentRectEdges[4];
  ObjectDragHandler m_contentRectEdgeHandlers[4];

  DraggablePolygon m_contentRectArea;
  ObjectDragHandler m_contentRectAreaHandler;

  DraggablePoint m_pageRectCorners[4];
  ObjectDragHandler m_pageRectCornerHandlers[4];

  DraggableLineSegment m_pageRectEdges[4];
  ObjectDragHandler m_pageRectEdgeHandlers[4];

  DraggablePolygon m_pageRectArea;
  ObjectDragHandler m_pageRectAreaHandler;

  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;

  /**
   * The context menu to be shown if there is no content box.
   */
  QMenu* m_noContentMenu;

  /**
   * The context menu to be shown if there exists a content box.
   */
  QMenu* m_haveContentMenu;

  /**
   * Content box in virtual image coordinates.
   */
  QRectF m_contentRect;
  QRectF m_pageRect;

  bool m_pageRectEnabled;
  bool m_pageRectReloadRequested;

  QSizeF m_minBoxSize;

  ContentMask m_contentMask;

  Qt::KeyboardModifier m_adjustmentVerticalModifier;
  Qt::KeyboardModifier m_adjustmentHorizontalModifier;
};
}  // namespace select_content
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_IMAGEVIEW_H_
