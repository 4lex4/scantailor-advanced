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

#ifndef PAGE_LAYOUT_IMAGEVIEW_H_
#define PAGE_LAYOUT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "ImageTransformation.h"
#include "InteractionHandler.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "DraggableObject.h"
#include "ObjectDragHandler.h"
#include "Alignment.h"
#include "intrusive_ptr.h"
#include "PageId.h"
#include "Guide.h"
#include <QTransform>
#include <QSizeF>
#include <QRectF>
#include <QPointF>
#include <QPoint>
#include <QMenu>
#include <interaction/DraggableLineSegment.h>

class Margins;

namespace page_layout {
class OptionsWidget;
class Settings;

class ImageView : public ImageViewBase, private InteractionHandler {
    Q_OBJECT
public:
    ImageView(const intrusive_ptr<Settings>& settings,
              const PageId& page_id,
              const QImage& image,
              const QImage& downscaled_image,
              const ImageTransformation& xform,
              const QRectF& adapted_content_rect,
              const OptionsWidget& opt_widget);

    ~ImageView() override;

signals:

    void invalidateThumbnail(const PageId& page_id);

    void invalidateAllThumbnails();

    void marginsSetLocally(const Margins& margins_mm);

public slots:

    void marginsSetExternally(const Margins& margins_mm);

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

    Proximity cornerProximity(int edge_mask, const QRectF* box, const QPointF& mouse_pos) const;

    Proximity edgeProximity(int edge_mask, const QRectF* box, const QPointF& mouse_pos) const;

    void dragInitiated(const QPointF& mouse_pos);

    void innerRectDragContinuation(int edge_mask, const QPointF& mouse_pos);

    void middleRectDragContinuation(int edge_mask, const QPointF& mouse_pos);

    void dragFinished();

    void recalcBoxesAndFit(const Margins& margins_mm);

    void updatePresentationTransform(FitMode fit_mode);

    void forceNonNegativeHardMargins(QRectF& middle_rect) const;

    Margins calcHardMarginsMM() const;

    void recalcOuterRect();

    QSizeF origRectToSizeMM(const QRectF& rect) const;

    AggregateSizeChanged commitHardMargins(const Margins& margins_mm);

    void invalidateThumbnails(AggregateSizeChanged agg_size_changed);

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

    intrusive_ptr<Settings> m_ptrSettings;

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
     * m_ptrSettings->getAggregateHardSizeMM() would return.
     *
     * \see m_committedAggregateHardSizeMM
     */
    QSizeF m_aggregateHardSizeMM;

    /**
     * \brief Aggregate (max width + max height) hard page size.
     *
     * This one is supposed to be the cached version of what
     * m_ptrSettings->getAggregateHardSizeMM() would return.
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
    QPointF m_lastContextMenuPos;
    int m_guideUnderMouse;
};
}  // namespace page_layout
#endif  // ifndef PAGE_LAYOUT_IMAGEVIEW_H_
