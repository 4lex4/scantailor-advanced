// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_IMAGEVIEW_H_
#define SCANTAILOR_PAGE_SPLIT_IMAGEVIEW_H_

#include <QPixmap>
#include "DragHandler.h"
#include "DraggableLineSegment.h"
#include "DraggablePoint.h"
#include "ImageId.h"
#include "ImageViewBase.h"
#include "ObjectDragHandler.h"
#include "PageLayout.h"
#include "UnremoveButton.h"
#include "ZoomHandler.h"
#include "intrusive_ptr.h"

class ImageTransformation;
class ProjectPages;
class PageInfo;
class QPointF;
class QRectF;
class QLineF;

namespace page_split {
class ImageView : public ImageViewBase, private InteractionHandler {
  Q_OBJECT
 public:
  ImageView(const QImage& image,
            const QImage& downscaledImage,
            const ImageTransformation& xform,
            const PageLayout& layout,
            intrusive_ptr<ProjectPages> pages,
            const ImageId& imageId,
            bool leftHalfRemoved,
            bool rightHalfRemoved);

  ~ImageView() override;

 signals:

  void invalidateThumbnail(const PageInfo& pageInfo);

  void pageLayoutSetLocally(const PageLayout& layout);

 public slots:

  void pageLayoutSetExternally(const PageLayout& layout);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

 private:
  void setupCuttersInteraction();

  QPointF handlePosition(int lineIdx, int handleIdx) const;

  void handleMoveRequest(int lineIdx, int handleIdx, const QPointF& pos);

  QLineF linePosition(int lineIdx) const;

  void lineMoveRequest(int lineIdx, QLineF line);

  void dragFinished();

  QPointF leftPageCenter() const;

  QPointF rightPageCenter() const;

  void unremoveLeftPage();

  void unremoveRightPage();

  /**
   * \return Page layout in widget coordinates.
   */
  PageLayout widgetLayout() const;

  /**
   * \return A Cutter line in widget coordinates.
   *
   * Depending on the current interaction state, the line segment
   * may end either shortly before the widget boundaries, or shortly
   * before the image boundaries.
   */
  QLineF widgetCutterLine(int lineIdx) const;

  /**
   * \return A Cutter line in virtual image coordinates.
   *
   * Unlike widgetCutterLine(), this one always ends shortly before
   * the image boundaries.
   */
  QLineF virtualCutterLine(int lineIdx) const;

  /**
   * Same as ImageViewBase::getVisibleWidgetRect(), except reduced
   * vertically to accomodate the height of line endpoint handles.
   */
  QRectF reducedWidgetArea() const;

  static QLineF customInscribedCutterLine(const QLineF& line, const QRectF& rect);

  intrusive_ptr<ProjectPages> m_pages;
  ImageId m_imageId;
  DraggablePoint m_handles[2][2];
  ObjectDragHandler m_handleInteractors[2][2];
  DraggableLineSegment m_lineSegments[2];
  ObjectDragHandler m_lineInteractors[2];
  UnremoveButton m_leftUnremoveButton;
  UnremoveButton m_rightUnremoveButton;
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;

  QPixmap m_handlePixmap;

  /**
   * Page layout in virtual image coordinates.
   */
  PageLayout m_virtLayout;

  bool m_leftPageRemoved;
  bool m_rightPageRemoved;
};
}  // namespace page_split
#endif  // ifndef SCANTAILOR_PAGE_SPLIT_IMAGEVIEW_H_
