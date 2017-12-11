/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef SELECT_CONTENT_IMAGEVIEW_H_
#define SELECT_CONTENT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "DraggablePoint.h"
#include "DraggableLineSegment.h"
#include "ObjectDragHandler.h"
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <interaction/DraggablePolygon.h>

class ImageTransformation;
class QMenu;

namespace select_content {
    class ImageView : public ImageViewBase, private InteractionHandler {
    Q_OBJECT
    public:
        /**
         * \p content_rect is in virtual image coordinates.
         */
        ImageView(QImage const& image,
                  QImage const& downscaled_image,
                  ImageTransformation const& xform,
                  QRectF const& content_rect,
                  QRectF const& page_rect,
                  bool page_rect_enabled);

        virtual ~ImageView();

    signals:

        void manualContentRectSet(QRectF const& content_rect);

        void manualPageRectSet(QRectF const& page_rect);

    private slots:

        void createContentBox();

        void removeContentBox();

    private:
        enum Edge {
            LEFT = 1,
            RIGHT = 2,
            TOP = 4,
            BOTTOM = 8
        };

        void onPaint(QPainter& painter, InteractionState const& interaction) override;

        void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) override;

        QPointF contentRectCornerPosition(int edge_mask) const;

        void contentRectCornerMoveRequest(int edge_mask, QPointF const& pos);

        QLineF contentRectEdgePosition(int edge) const;

        void contentRectEdgeMoveRequest(int edge, QLineF const& line);

        void contentRectDragFinished();

        QPointF pageRectCornerPosition(int edge_mask) const;

        void pageRectCornerMoveRequest(int edge_mask, QPointF const& pos);

        QLineF pageRectEdgePosition(int edge) const;

        void pageRectEdgeMoveRequest(int edge, QLineF const& line);

        void pageRectDragFinished();

        void forceInsideImage(QRectF& widget_rect, int edge_mask) const;

        void forcePageRectDescribeContent();

        QRectF contentRectPosition() const;

        void contentRectMoveRequest(QPolygonF const& pos);

        QRectF pageRectPosition() const;

        void pageRectMoveRequest(QPolygonF const& pos);

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
        QMenu* m_pNoContentMenu;

        /**
         * The context menu to be shown if there exists a content box.
         */
        QMenu* m_pHaveContentMenu;

        /**
         * Content box in virtual image coordinates.
         */
        QRectF m_contentRect;
        QRectF m_pageRect;

        bool m_pageRectEnabled;
        bool m_pageRectReloadRequested;

        QSizeF m_minBoxSize;
    };
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_IMAGEVIEW_H_
