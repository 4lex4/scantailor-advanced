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

#ifndef IMAGEVIEWBASE_H_
#define IMAGEVIEWBASE_H_

#include <QAbstractScrollArea>
#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QTimer>
#include <QTransform>
#include <QWidget>
#include <Qt>
#include "ImagePixmapUnion.h"
#include "ImageViewInfoProvider.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "Margins.h"
#include "intrusive_ptr.h"

class QPainter;
class BackgroundExecutor;
class ImagePresentation;

/**
 * \brief The base class for widgets that display and manipulate images.
 *
 * This class operates with 3 coordinate systems:
 * \li Image coordinates, where m_image.rect() is defined.
 * \li Pixmap coordinates, where m_pixmap.rect() is defined.
 *     m_pixmap is constructed from a downscaled version of m_image.
 * \li Virtual image coordinates.  We need them because we are not
 *     displaying m_image as is.  Instead, we display a pre-transformed
 *     version of it.  So, the virtual image coordinates reference the
 *     pixels of an imaginary image that we would get if we actually
 *     pre-transformed m_image the way we want.
 * \li Widget coordinates, where this->rect() is defined.
 *
 * \see m_pixmapToImage, m_imageToVirt, m_virtualToWidget, m_widgetToVirtual.
 */
class ImageViewBase : public QAbstractScrollArea {
  Q_OBJECT
 public:
  enum FocalPointMode { CENTER_IF_FITS, DONT_CENTER };

  /**
   * \brief ImageViewBase constructor.
   *
   * \param image The image to display.
   * \param downscaled_version The downscaled version of \p image.
   *        If it's null, it will be created automatically.
   *        The exact scale doesn't matter.
   *        The whole idea of having a downscaled version is
   *        to speed up real-time rendering of high-resolution
   *        images.  Note that the delayed high quality transform
   *        operates on the original image, not the downscaled one.
   * \param presentation Specifies transformation from image
   *        pixel coordinates to virtual image coordinates, along
   *        with some other properties.
   * \param margins Reserve extra space near the widget borders.
   *        The units are widget pixels.  This reserved area may
   *        be used for custom drawing or custom controls.
   */
  ImageViewBase(const QImage& image,
                const ImagePixmapUnion& downscaled_version,
                const ImagePresentation& presentation,
                const Margins& margins = Margins());

  ~ImageViewBase() override;

  /**
   * The idea behind this accessor is being able to share a single
   * downscaled pixmap between multiple image views.
   */
  const QPixmap& downscaledPixmap() const { return m_pixmap; }

  /**
   * \brief Enable or disable the high-quality transform.
   */
  void hqTransformSetEnabled(bool enabled);

  /**
   * \brief A stand-alone function to create a downscaled image
   *        to be passed to the constructor.
   *
   * The point of using this function instead of letting
   * the constructor do the job is that this function may
   * be called from a background thread, while the constructor
   * can't.
   *
   * \param image The input image, not null, and with DPI set correctly.
   * \return The image downscaled by an unspecified degree.
   */
  static QImage createDownscaledImage(const QImage& image);

  InteractionHandler& rootInteractionHandler() { return m_rootInteractionHandler; }

  InteractionState& interactionState() { return m_interactionState; }

  const InteractionState& interactionState() const { return m_interactionState; }

  const QTransform& imageToVirtual() const { return m_imageToVirtual; }

  const QTransform& virtualToImage() const { return m_virtualToImage; }

  const QTransform& virtualToWidget() const { return m_virtualToWidget; }

  const QTransform& widgetToVirtual() const { return m_widgetToVirtual; }

  QTransform imageToWidget() const { return m_imageToVirtual * m_virtualToWidget; }

  QTransform widgetToImage() const { return m_widgetToVirtual * m_virtualToImage; }

  void update() { viewport()->update(); }

  const QRectF& virtualDisplayRect() const { return m_virtualDisplayArea; }

  /**
   * Get the bounding box of the image as it appears on the screen,
   * in widget coordinates.
   */
  QRectF getOccupiedWidgetRect() const;

  /**
   * \brief A better version of setStatusTip().
   *
   * Unlike setStatusTip(), this method will display the tooltip
   * immediately, not when the mouse enters the widget next time.
   */
  void ensureStatusTip(const QString& status_tip);

  /**
   * \brief Get the focal point in widget coordinates.
   *
   * The typical usage pattern for this function is:
   * \code
   * QPointF fp(obj.getWidgetFocalPoint());
   * obj.setWidgetFocalPoint(fp + delta);
   * \endcode
   * As a result, the image will be moved by delta widget pixels.
   */
  QPointF getWidgetFocalPoint() const { return m_widgetFocalPoint; }

  /**
   * \brief Set the focal point in widget coordinates.
   *
   * This one may be used for unrestricted dragging (with Shift button).
   *
   * \see getWidgetFocalPoint()
   */
  void setWidgetFocalPoint(const QPointF& widget_fp);

  /**
   * \brief Set the focal point in widget coordinates, after adjustring
   *        it to avoid wasting of widget space.
   *
   * This one may be used for resticted dragging (the default one in ST).
   *
   * \see getWidgetFocalPoint()
   * \see setWidgetFocalPoint()
   */
  void adjustAndSetWidgetFocalPoint(const QPointF& widget_fp);

  /**
   * \brief Sets the widget focal point and recalculates the pixmap focal
   *        focal point so that the image is not moved on screen.
   */
  void setWidgetFocalPointWithoutMoving(QPointF new_widget_fp);

  /**
   * \brief Updates image-to-virtual and recalculates
   *        virtual-to-widget transformations.
   */
  void updateTransform(const ImagePresentation& presentation);

  /**
   * \brief Same as updateTransform(), but adjusts the focal point
   *        to improve screen space usage.
   */
  void updateTransformAndFixFocalPoint(const ImagePresentation& presentation, FocalPointMode mode);

  /**
   * \brief Same as updateTransform(), but preserves the visual image scale.
   */
  void updateTransformPreservingScale(const ImagePresentation& presentation);

  /**
   * \brief Sets the zoom level.
   *
   * Zoom level 1.0 means such a zoom that makes the image fit the widget.
   * Zooming will take into account the current widget and pixmap focal
   * points.  To zoom to a specific point, for example the mouse position,
   * call setWidgetFocalPointWithoutMoving() first.
   */
  void setZoomLevel(double zoom);

  /**
   * \brief Returns the current zoom level.
   * \see setZoomLevel()
   */
  double zoomLevel() const { return m_zoom; }

  /**
   * The image is considered ideally positioned when as little as possible
   * screen space is wasted.
   *
   * \param pixel_length The euclidean distance in widget pixels to move the image.
   *        Will be clipped if it's more than required to reach the ideal position.
   */
  void moveTowardsIdealPosition(double pixel_length);

  static BackgroundExecutor& backgroundExecutor();

  ImageViewInfoProvider& infoProvider();

 protected:
  void paintEvent(QPaintEvent* event) override;

  void keyPressEvent(QKeyEvent* event) override;

  void keyReleaseEvent(QKeyEvent* event) override;

  void mousePressEvent(QMouseEvent* event) override;

  void mouseReleaseEvent(QMouseEvent* event) override;

  void mouseDoubleClickEvent(QMouseEvent* event) override;

  void mouseMoveEvent(QMouseEvent* event) override;

  void wheelEvent(QWheelEvent* event) override;

  void contextMenuEvent(QContextMenuEvent* event) override;

  void resizeEvent(QResizeEvent* event) override;

  void enterEvent(QEvent* event) override;

  void leaveEvent(QEvent* event) override;

  void showEvent(QShowEvent* event) override;

  /**
   * Returns the maximum viewport size (as if scrollbars are hidden)
   * reduced by margins.
   */
  QRectF maxViewportRect() const;

  virtual void updateCursorPos(const QPointF& pos);

  virtual void updatePhysSize();

 private slots:

  void initiateBuildingHqVersion();

  void updateScrollBars();

  void reactToScrollBars();

 private:
  class HqTransformTask;
  class TempFocalPointAdjuster;

  class TransformChangeWatcher;

  QRectF dynamicViewportRect() const;

  void transformChanged();

  void updateWidgetTransform();

  void updateWidgetTransformAndFixFocalPoint(FocalPointMode mode);

  QPointF getIdealWidgetFocalPoint(FocalPointMode mode) const;

  void setNewWidgetFP(QPointF widget_fp, bool update = false);

  void adjustAndSetNewWidgetFP(QPointF proposed_widget_fp, bool update = false);

  QPointF centeredWidgetFocalPoint() const;

  bool validateHqPixmap() const;

  void scheduleHqVersionRebuild();

  void hqVersionBuilt(const QPoint& origin, const QImage& image);

  void updateStatusTipAndCursor();

  void updateStatusTip();

  void updateCursor();

  void maybeQueueRedraw();

  InteractionHandler m_rootInteractionHandler;

  InteractionState m_interactionState;

  /**
   * The client-side image.  Used to build a high-quality version
   * for delayed rendering.
   */
  QImage m_image;

  /**
   * This timer is used for delaying the construction of
   * a high quality image version.
   */
  QTimer m_timer;

  /**
   * The image handle.  Note that the actual data of a QPixmap lives
   * in another process on most platforms.
   */
  QPixmap m_pixmap;

  /**
   * The high quality, pre-transformed version of m_pixmap.
   */
  QPixmap m_hqPixmap;

  /**
   * The position, in widget coordinates, where m_hqPixmap is to be drawn.
   */
  QPoint m_hqPixmapPos;

  /**
   * The transformation used to build m_hqPixmap.
   * It's used to detect if m_hqPixmap needs to be rebuild.
   */
  QTransform m_hqXform;

  /**
   * Used to check if we need to extend the delay before building m_hqPixmap.
   */
  QTransform m_potentialHqXform;

  /**
   * The ID (QImage::cacheKey()) of the image that was used
   * to build m_hqPixmap.  It's used to detect if m_hqPixmap
   * needs to be rebuilt.
   */
  qint64 m_hqSourceId;

  /**
   * The pending (if any) high quality transformation task.
   */
  intrusive_ptr<HqTransformTask> m_ptrHqTransformTask;

  /**
   * Transformation from m_pixmap coordinates to m_image coordinates.
   */
  QTransform m_pixmapToImage;

  /**
   * The area of the virtual image to be displayed.
   * Everything outside of it will be cropped.
   */
  QPolygonF m_virtualImageCropArea;

  /**
   * The area in virtual image coordinates to be displayed.
   * The idea is that it can be larger than m_virtualImageCropArea
   * to reserve space for custom drawing or controls.
   */
  QRectF m_virtualDisplayArea;

  /**
   * A transformation from original to virtual image coordinates.
   */
  QTransform m_imageToVirtual;

  /**
   * A transformation from virtual to original image coordinates.
   */
  QTransform m_virtualToImage;

  /**
   * Transformation from virtual image coordinates to widget coordinates.
   */
  QTransform m_virtualToWidget;

  /**
   * Transformation from widget coordinates to virtual image coordinates.
   */
  QTransform m_widgetToVirtual;

  /**
   * Transforms scroll bar values to corresponding positions of the display
   * area (its central point) in widget coordinates.
   */
  QTransform m_scrollTransform;

  /**
   * An arbitrary point in widget coordinates that corresponds
   * to m_pixmapFocalPoint in m_pixmap coordinates.
   * Moving m_widgetFocalPoint followed by updateWidgetTransform()
   * will cause the image to move on screen.
   */
  QPointF m_widgetFocalPoint;

  /**
   * An arbitrary point in m_pixmap coordinates that corresponds
   * to m_widgetFocalPoint in widget coordinates.
   * Unlike m_widgetFocalPoint, this one is not supposed to be
   * moved independently.  It's supposed to moved together with
   * m_widgetFocalPoint for zooming into a specific position.
   */
  QPointF m_pixmapFocalPoint;

  /**
   * Used to distinguish between resizes induced by scrollbars (dis)appearing
   * and other factors.
   */
  QSize m_lastMaximumViewportSize;

  /**
   * The number of pixels to be left blank at each side of the widget.
   */
  Margins m_margins;

  /**
   * The zoom factor.  A value of 1.0 corresponds to fit-to-widget zoom.
   */
  double m_zoom;

  /**
   * This timer is used for tracking the cursor position.
   */
  QTimer m_cursorTrackerTimer;

  /**
   * Current mouse pos in widget coordinates.
   */
  QPointF m_cursorPos;

  int m_transformChangeWatchersActive;

  int m_ignoreScrollEvents;

  int m_ignoreResizeEvents;

  bool m_hqTransformEnabled;

  ImageViewInfoProvider m_infoProvider;
};


#endif  // ifndef IMAGEVIEWBASE_H_
