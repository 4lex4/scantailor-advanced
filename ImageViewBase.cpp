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

#include "ImageViewBase.h"
#include <QApplication>
#include <QGLWidget>
#include <QMouseEvent>
#include <QPaintEngine>
#include <QPointer>
#include <QScrollBar>
#include <QSettings>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include "BackgroundExecutor.h"
#include "ColorSchemeManager.h"
#include "Dpm.h"
#include "ImagePresentation.h"
#include "OpenGLSupport.h"
#include "PixmapRenderer.h"
#include "ScopedIncDec.h"
#include "UnitsProvider.h"
#include "Utils.h"
#include "imageproc/PolygonUtils.h"
#include "imageproc/Transform.h"

using namespace imageproc;

class ImageViewBase::HqTransformTask : public AbstractCommand<intrusive_ptr<AbstractCommand<void>>>, public QObject {
  DECLARE_NON_COPYABLE(HqTransformTask)

 public:
  HqTransformTask(ImageViewBase* image_view, const QImage& image, const QTransform& xform, const QSize& target_size);

  void cancel() { m_ptrResult->cancel(); }

  const bool isCancelled() const { return m_ptrResult->isCancelled(); }

  intrusive_ptr<AbstractCommand<void>> operator()() override;

 private:
  class Result : public AbstractCommand<void> {
   public:
    explicit Result(ImageViewBase* image_view);

    void setData(const QPoint& origin, const QImage& hq_image);

    void cancel() { m_cancelFlag.fetchAndStoreRelaxed(1); }

    bool isCancelled() const { return m_cancelFlag.fetchAndAddRelaxed(0) != 0; }

    void operator()() override;

   private:
    QPointer<ImageViewBase> m_ptrImageView;
    QPoint m_origin;
    QImage m_hqImage;
    mutable QAtomicInt m_cancelFlag;
  };


  intrusive_ptr<Result> m_ptrResult;
  QImage m_image;
  QTransform m_xform;
  QSize m_targetSize;
};


/**
 * \brief Temporarily adjust the widget focal point, then change it back.
 *
 * When adjusting and restoring the widget focal point, the pixmap
 * focal point is recalculated accordingly.
 */
class ImageViewBase::TempFocalPointAdjuster {
 public:
  /**
   * Change the widget focal point to obj.centeredWidgetFocalPoint().
   */
  explicit TempFocalPointAdjuster(ImageViewBase& obj);

  /**
   * Change the widget focal point to \p temp_widget_fp
   */
  TempFocalPointAdjuster(ImageViewBase& obj, QPointF temp_widget_fp);

  /**
   * Restore the widget focal point.
   */
  ~TempFocalPointAdjuster();

 private:
  ImageViewBase& m_rObj;
  QPointF m_origWidgetFP;
};


class ImageViewBase::TransformChangeWatcher {
 public:
  explicit TransformChangeWatcher(ImageViewBase& owner);

  ~TransformChangeWatcher();

 private:
  ImageViewBase& m_rOwner;
  QTransform m_imageToVirtual;
  QTransform m_virtualToWidget;
  QRectF m_virtualDisplayArea;
};


ImageViewBase::ImageViewBase(const QImage& image,
                             const ImagePixmapUnion& downscaled_version,
                             const ImagePresentation& presentation,
                             const Margins& margins)
    : m_image(image),
      m_virtualImageCropArea(presentation.cropArea()),
      m_virtualDisplayArea(presentation.displayArea()),
      m_imageToVirtual(presentation.transform()),
      m_virtualToImage(presentation.transform().inverted()),
      m_lastMaximumViewportSize(maximumViewportSize()),
      m_margins(margins),
      m_zoom(1.0),
      m_transformChangeWatchersActive(0),
      m_ignoreScrollEvents(0),
      m_ignoreResizeEvents(0),
      m_hqTransformEnabled(true),
      m_infoProvider(Dpm(m_image)) {
  /* For some reason, the default viewport fills background with
   * a color different from QPalette::Window. Here we make it not
   * fill it at all, assuming QMainWindow will do that anyway
   * (with the correct color). Note that an attempt to do the same
   * to an OpenGL viewport produces "black hole" artefacts. Therefore,
   * we do this before setting an OpenGL viewport rather than after.
   */
  viewport()->setAutoFillBackground(false);

  if (QSettings().value("settings/use_3d_acceleration", false) != false) {
    if (OpenGLSupport::supported()) {
      QGLFormat format;
      format.setSampleBuffers(true);
      format.setStencil(true);
      format.setAlpha(true);
      format.setRgba(true);
      format.setDepth(false);

      // Most of hardware refuses to work for us with direct rendering enabled.
      format.setDirectRendering(false);

      setViewport(new QGLWidget(format));
    }
  }

  setFrameShape(QFrame::NoFrame);
  viewport()->setFocusPolicy(Qt::WheelFocus);

  if (downscaled_version.isNull()) {
    m_pixmap = QPixmap::fromImage(createDownscaledImage(image));
  } else if (downscaled_version.pixmap().isNull()) {
    m_pixmap = QPixmap::fromImage(downscaled_version.image());
  } else {
    m_pixmap = downscaled_version.pixmap();
  }

  m_pixmapToImage.scale((double) m_image.width() / m_pixmap.width(), (double) m_image.height() / m_pixmap.height());

  m_widgetFocalPoint = centeredWidgetFocalPoint();
  m_pixmapFocalPoint = m_virtualToImage.map(virtualDisplayRect().center());

  m_timer.setSingleShot(true);
  m_timer.setInterval(150);  // msec
  connect(&m_timer, SIGNAL(timeout()), this, SLOT(initiateBuildingHqVersion()));

  setMouseTracking(true);
  m_cursorTrackerTimer.setSingleShot(true);
  m_cursorTrackerTimer.setInterval(150);  // msec
  connect(&m_cursorTrackerTimer, &QTimer::timeout, [this]() {
    QPointF cursorPos;
    if (!m_cursorPos.isNull()) {
      cursorPos = m_widgetToVirtual.map(m_cursorPos) - m_virtualImageCropArea.boundingRect().topLeft();
    }
    m_infoProvider.setMousePos(cursorPos);
  });

  updatePhysSize();

  updateWidgetTransformAndFixFocalPoint(CENTER_IF_FITS);

  interactionState().setDefaultStatusTip(tr("Use the mouse wheel or +/- to zoom.  When zoomed, dragging is possible."));
  ensureStatusTip(interactionState().statusTip());

  connect(horizontalScrollBar(), SIGNAL(sliderReleased()), SLOT(updateScrollBars()));
  connect(verticalScrollBar(), SIGNAL(sliderReleased()), SLOT(updateScrollBars()));
  connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(reactToScrollBars()));
  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(reactToScrollBars()));
}

ImageViewBase::~ImageViewBase() = default;

void ImageViewBase::hqTransformSetEnabled(const bool enabled) {
  if (!enabled && m_hqTransformEnabled) {
    // Turning off.
    m_hqTransformEnabled = false;
    if (m_ptrHqTransformTask) {
      m_ptrHqTransformTask->cancel();
      m_ptrHqTransformTask.reset();
    }
    if (!m_hqPixmap.isNull()) {
      m_hqPixmap = QPixmap();
      update();
    }
  } else if (enabled && !m_hqTransformEnabled) {
    // Turning on.
    m_hqTransformEnabled = true;
    update();
  }
}

QImage ImageViewBase::createDownscaledImage(const QImage& image) {
  assert(!image.isNull());

  // Original and downscaled DPM.
  const Dpm o_dpm(image);
  const Dpm d_dpm(Dpi(200, 200));

  const int o_w = image.width();
  const int o_h = image.height();

  int d_w = o_w * d_dpm.horizontal() / o_dpm.horizontal();
  int d_h = o_h * d_dpm.vertical() / o_dpm.vertical();
  d_w = qBound(1, d_w, o_w);
  d_h = qBound(1, d_h, o_h);

  if ((d_w * 1.2 > o_w) || (d_h * 1.2 > o_h)) {
    // Sizes are close - no point in downscaling.
    return image;
  }

  QTransform xform;
  xform.scale((double) d_w / o_w, (double) d_h / o_h);

  return transform(image, xform, QRect(0, 0, d_w, d_h), OutsidePixels::assumeColor(Qt::white));
}

QRectF ImageViewBase::maxViewportRect() const {
  const QRectF viewport_rect(QPointF(0, 0), maximumViewportSize());
  QRectF r(viewport_rect);
  r.adjust(m_margins.left(), m_margins.top(), -m_margins.right(), -m_margins.bottom());
  if (r.isEmpty()) {
    return QRectF(viewport_rect.center(), viewport_rect.center());
  }

  return r;
}

QRectF ImageViewBase::dynamicViewportRect() const {
  const QRectF viewport_rect(viewport()->rect());
  QRectF r(viewport_rect);
  r.adjust(m_margins.left(), m_margins.top(), -m_margins.right(), -m_margins.bottom());
  if (r.isEmpty()) {
    return QRectF(viewport_rect.center(), viewport_rect.center());
  }

  return r;
}

QRectF ImageViewBase::getOccupiedWidgetRect() const {
  const QRectF widget_rect(m_virtualToWidget.mapRect(virtualDisplayRect()));

  return widget_rect.intersected(dynamicViewportRect());
}

void ImageViewBase::setWidgetFocalPoint(const QPointF& widget_fp) {
  setNewWidgetFP(widget_fp, /*update =*/true);
}

void ImageViewBase::adjustAndSetWidgetFocalPoint(const QPointF& widget_fp) {
  adjustAndSetNewWidgetFP(widget_fp, /*update=*/true);
}

void ImageViewBase::setZoomLevel(double zoom) {
  if (m_zoom != zoom) {
    m_zoom = zoom;
    updateWidgetTransform();
    update();
  }
}

void ImageViewBase::moveTowardsIdealPosition(const double pixel_length) {
  if (pixel_length <= 0) {
    // The name implies we are moving *towards* the ideal position.
    return;
  }

  const QPointF ideal_widget_fp(getIdealWidgetFocalPoint(CENTER_IF_FITS));
  if (ideal_widget_fp == m_widgetFocalPoint) {
    return;
  }

  QPointF vec(ideal_widget_fp - m_widgetFocalPoint);
  const double max_length = std::sqrt(vec.x() * vec.x() + vec.y() * vec.y());
  if (pixel_length >= max_length) {
    m_widgetFocalPoint = ideal_widget_fp;
  } else {
    vec *= pixel_length / max_length;
    m_widgetFocalPoint += vec;
  }

  updateWidgetTransform();
  update();
}

void ImageViewBase::updateTransform(const ImagePresentation& presentation) {
  const TransformChangeWatcher watcher(*this);
  const TempFocalPointAdjuster temp_fp(*this);

  m_imageToVirtual = presentation.transform();
  m_virtualToImage = m_imageToVirtual.inverted();
  m_virtualImageCropArea = presentation.cropArea();
  m_virtualDisplayArea = presentation.displayArea();

  updateWidgetTransform();
  update();
  updatePhysSize();
}

void ImageViewBase::updateTransformAndFixFocalPoint(const ImagePresentation& presentation, const FocalPointMode mode) {
  const TransformChangeWatcher watcher(*this);
  const TempFocalPointAdjuster temp_fp(*this);

  m_imageToVirtual = presentation.transform();
  m_virtualToImage = m_imageToVirtual.inverted();
  m_virtualImageCropArea = presentation.cropArea();
  m_virtualDisplayArea = presentation.displayArea();

  updateWidgetTransformAndFixFocalPoint(mode);
  update();
  updatePhysSize();
}

void ImageViewBase::updateTransformPreservingScale(const ImagePresentation& presentation) {
  const TransformChangeWatcher watcher(*this);
  const TempFocalPointAdjuster temp_fp(*this);

  // An arbitrary line in image coordinates.
  const QLineF image_line(0.0, 0.0, 1.0, 1.0);

  const QLineF widget_line_before((m_imageToVirtual * m_virtualToWidget).map(image_line));

  m_imageToVirtual = presentation.transform();
  m_virtualToImage = m_imageToVirtual.inverted();
  m_virtualImageCropArea = presentation.cropArea();
  m_virtualDisplayArea = presentation.displayArea();

  updateWidgetTransform();

  const QLineF widget_line_after((m_imageToVirtual * m_virtualToWidget).map(image_line));

  m_zoom *= widget_line_before.length() / widget_line_after.length();
  updateWidgetTransform();

  update();
  updatePhysSize();
}

void ImageViewBase::ensureStatusTip(const QString& status_tip) {
  const QString cur_status_tip(statusTip());
  if (cur_status_tip.constData() == status_tip.constData()) {
    return;
  }
  if (cur_status_tip == status_tip) {
    return;
  }

  viewport()->setStatusTip(status_tip);

  if (viewport()->underMouse()) {
    // Note that setStatusTip() alone is not enough,
    // as it's only taken into account when the mouse
    // enters the widget.
    // Also note that we use postEvent() rather than sendEvent(),
    // because sendEvent() may immediately process other events.
    QApplication::postEvent(viewport(), new QStatusTipEvent(status_tip));
  }
}

void ImageViewBase::paintEvent(QPaintEvent* event) {
  QPainter painter(viewport());
  painter.save();

  const double xscale = m_virtualToWidget.m11();

  // Width of a source pixel in mm, as it's displayed on screen.
  const double pixel_width = widthMM() * xscale / width();

  // Make clipping smooth.
  painter.setRenderHint(QPainter::Antialiasing, true);

  // Disable antialiasing for large zoom levels.
  painter.setRenderHint(QPainter::SmoothPixmapTransform, pixel_width < 0.5);

  if (validateHqPixmap()) {
    // HQ pixmap maps one to one to screen pixels, so antialiasing is not necessary.
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    QPainterPath clip_path;
    clip_path.addPolygon(m_virtualToWidget.map(m_virtualImageCropArea));
    painter.setClipPath(clip_path);

    painter.drawPixmap(m_hqPixmapPos, m_hqPixmap);
  } else {
    scheduleHqVersionRebuild();

    const QTransform pixmap_to_virtual(m_pixmapToImage * m_imageToVirtual);
    painter.setWorldTransform(pixmap_to_virtual * m_virtualToWidget);

    QPainterPath clip_path;
    clip_path.addPolygon(pixmap_to_virtual.inverted().map(m_virtualImageCropArea));
    painter.setClipPath(clip_path);

    PixmapRenderer::drawPixmap(painter, m_pixmap);
  }

  painter.restore();

  painter.setWorldTransform(m_virtualToWidget);

  m_interactionState.resetProximity();
  if (!m_interactionState.captured()) {
    m_rootInteractionHandler.proximityUpdate(QPointF(0.5, 0.5) + mapFromGlobal(QCursor::pos()), m_interactionState);
    updateStatusTipAndCursor();
  }

  m_rootInteractionHandler.paint(painter, m_interactionState);
  maybeQueueRedraw();
}  // ImageViewBase::paintEvent

void ImageViewBase::keyPressEvent(QKeyEvent* event) {
  event->setAccepted(false);
  m_rootInteractionHandler.keyPressEvent(event, m_interactionState);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
}

void ImageViewBase::keyReleaseEvent(QKeyEvent* event) {
  event->setAccepted(false);
  m_rootInteractionHandler.keyReleaseEvent(event, m_interactionState);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
}

void ImageViewBase::mousePressEvent(QMouseEvent* event) {
  m_interactionState.resetProximity();
  if (!m_interactionState.captured()) {
    m_rootInteractionHandler.proximityUpdate(QPointF(0.5, 0.5) + event->pos(), m_interactionState);
  }

  event->setAccepted(false);
  m_rootInteractionHandler.mousePressEvent(event, m_interactionState);
  event->setAccepted(true);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
}

void ImageViewBase::mouseReleaseEvent(QMouseEvent* event) {
  m_interactionState.resetProximity();
  if (!m_interactionState.captured()) {
    m_rootInteractionHandler.proximityUpdate(QPointF(0.5, 0.5) + event->pos(), m_interactionState);
  }

  event->setAccepted(false);
  m_rootInteractionHandler.mouseReleaseEvent(event, m_interactionState);
  event->setAccepted(true);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
}

void ImageViewBase::mouseDoubleClickEvent(QMouseEvent* event) {
  m_interactionState.resetProximity();
  if (!m_interactionState.captured()) {
    m_rootInteractionHandler.proximityUpdate(QPointF(0.5, 0.5) + event->pos(), m_interactionState);
  }

  event->setAccepted(false);
  m_rootInteractionHandler.mouseDoubleClickEvent(event, m_interactionState);
  event->setAccepted(true);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
}

void ImageViewBase::mouseMoveEvent(QMouseEvent* event) {
  m_interactionState.resetProximity();
  if (!m_interactionState.captured()) {
    m_rootInteractionHandler.proximityUpdate(QPointF(0.5, 0.5) + event->pos(), m_interactionState);
  }

  event->setAccepted(false);
  m_rootInteractionHandler.mouseMoveEvent(event, m_interactionState);
  event->setAccepted(true);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
  updateCursorPos(event->localPos());
}

void ImageViewBase::wheelEvent(QWheelEvent* event) {
  event->setAccepted(false);
  m_rootInteractionHandler.wheelEvent(event, m_interactionState);
  event->setAccepted(true);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
}

void ImageViewBase::contextMenuEvent(QContextMenuEvent* event) {
  event->setAccepted(false);
  m_rootInteractionHandler.contextMenuEvent(event, m_interactionState);
  event->setAccepted(true);
  updateStatusTipAndCursor();
  maybeQueueRedraw();
}

void ImageViewBase::resizeEvent(QResizeEvent* event) {
  QAbstractScrollArea::resizeEvent(event);

  if (m_ignoreResizeEvents) {
    return;
  }

  const ScopedIncDec<int> guard(m_ignoreScrollEvents);

  if (maximumViewportSize() != m_lastMaximumViewportSize) {
    m_lastMaximumViewportSize = maximumViewportSize();
    m_widgetFocalPoint = centeredWidgetFocalPoint();
    updateWidgetTransform();
  } else {
    const TransformChangeWatcher watcher(*this);
    const TempFocalPointAdjuster temp_fp(*this, QPointF(0, 0));
    updateTransformPreservingScale(ImagePresentation(m_imageToVirtual, m_virtualImageCropArea, m_virtualDisplayArea));
  }
}

void ImageViewBase::enterEvent(QEvent* event) {
  viewport()->setFocus();
  QAbstractScrollArea::enterEvent(event);
}

void ImageViewBase::leaveEvent(QEvent* event) {
  updateCursorPos(QPointF());
  QAbstractScrollArea::leaveEvent(event);
}

void ImageViewBase::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);

  if (auto* mainWindow = dynamic_cast<QMainWindow*>(window())) {
    if (auto* infoObserver = Utils::castOrFindChild<ImageViewInfoObserver*>(mainWindow->statusBar())) {
      infoObserver->setInfoProvider(&infoProvider());
    }
  }
}

/**
 * Called when any of the transformations change.
 */
void ImageViewBase::transformChanged() {
  updateScrollBars();
}

void ImageViewBase::updateScrollBars() {
  if (verticalScrollBar()->isSliderDown() || horizontalScrollBar()->isSliderDown()) {
    return;
  }

  const ScopedIncDec<int> guard1(m_ignoreScrollEvents);
  const ScopedIncDec<int> guard2(m_ignoreResizeEvents);

  const QRectF picture(m_virtualToWidget.mapRect(virtualDisplayRect()));
  const QPointF viewport_center(maxViewportRect().center());
  const QPointF picture_center(picture.center());
  QRectF viewport(maxViewportRect());

  // Introduction of one scrollbar will decrease the available size in
  // another direction, which may cause a scrollbar in that direction
  // to become necessary.  For this reason, we have a loop here.
  for (int i = 0; i < 2; ++i) {
    const double xval = picture_center.x();
    double xmin, xmax;  // Minimum and maximum positions for picture center.
    if (picture_center.x() < viewport_center.x()) {
      xmin = std::min<double>(xval, viewport.right() - 0.5 * picture.width());
      xmax = std::max<double>(viewport_center.x(), viewport.left() + 0.5 * picture.width());
    } else {
      xmax = std::max<double>(xval, viewport.left() + 0.5 * picture.width());
      xmin = std::min<double>(viewport_center.x(), viewport.right() - 0.5 * picture.width());
    }

    const double yval = picture_center.y();
    double ymin, ymax;  // Minimum and maximum positions for picture center.
    if (picture_center.y() < viewport_center.y()) {
      ymin = std::min<double>(yval, viewport.bottom() - 0.5 * picture.height());
      ymax = std::max<double>(viewport_center.y(), viewport.top() + 0.5 * picture.height());
    } else {
      ymax = std::max<double>(yval, viewport.top() + 0.5 * picture.height());
      ymin = std::min<double>(viewport_center.y(), viewport.bottom() - 0.5 * picture.height());
    }

    const auto xrange = (int) std::ceil(xmax - xmin);
    const auto yrange = (int) std::ceil(ymax - ymin);
    const int xfirst = 0;
    const int xlast = xrange - 1;
    const int yfirst = 0;
    const int ylast = yrange - 1;

    // We are going to map scrollbar coordinates to widget coordinates
    // of the central point of the display area using a linear function.
    // f(x) = ax + b

    // xmin = xa * xlast + xb
    // xmax = xa * xfirst + xb
    const double xa = (xfirst == xlast) ? 1 : (xmax - xmin) / (xfirst - xlast);
    const double xb = xmax - xa * xfirst;
    const double ya = (yfirst == ylast) ? 1 : (ymax - ymin) / (yfirst - ylast);
    const double yb = ymax - ya * yfirst;

    // Inverse transformation.
    // xlast = ixa * xmin + ixb
    // xfirst = ixa * xmax + ixb
    const double ixa = (xmax == xmin) ? 1 : (xfirst - xlast) / (xmax - xmin);
    const double ixb = xfirst - ixa * xmax;
    const double iya = (ymax == ymin) ? 1 : (yfirst - ylast) / (ymax - ymin);
    const double iyb = yfirst - iya * ymax;

    m_scrollTransform.setMatrix(xa, 0, 0, 0, ya, 0, xb, yb, 1);

    const int xcur = qRound(ixa * xval + ixb);
    const int ycur = qRound(iya * yval + iyb);

    horizontalScrollBar()->setRange(xfirst, xlast);
    verticalScrollBar()->setRange(yfirst, ylast);

    horizontalScrollBar()->setValue(xcur);
    verticalScrollBar()->setValue(ycur);

    horizontalScrollBar()->setPageStep(qRound(viewport.width()));
    verticalScrollBar()->setPageStep(qRound(viewport.height()));
    // XXX: a hack to force immediate update of viewport()->rect(),
    // which is used by dynamicViewportRect() below.
    // Note that it involves a resize event being sent not only to
    // the viewport, but for some reason also to the containing
    // QAbstractScrollArea, that is to this object.
    setHorizontalScrollBarPolicy(horizontalScrollBarPolicy());

    const QRectF old_viewport(viewport);
    viewport = dynamicViewportRect();
    if (viewport == old_viewport) {
      break;
    }
  }
}  // ImageViewBase::updateScrollBars

void ImageViewBase::reactToScrollBars() {
  if (m_ignoreScrollEvents) {
    return;
  }

  const TransformChangeWatcher watcher(*this);

  const QPointF raw_position(horizontalScrollBar()->value(), verticalScrollBar()->value());
  const QPointF new_fp(m_scrollTransform.map(raw_position));
  const QPointF old_fp(getWidgetFocalPoint());

  m_pixmapFocalPoint = m_virtualToImage.map(m_virtualDisplayArea.center());
  m_widgetFocalPoint = new_fp;
  updateWidgetTransform();

  setWidgetFocalPointWithoutMoving(old_fp);
}

/**
 * Updates m_virtualToWidget and m_widgetToVirtual.\n
 * To be called whenever any of the following is modified:
 * m_imageToVirt, m_widgetFocalPoint, m_pixmapFocalPoint, m_zoom.
 * Modifying both m_widgetFocalPoint and m_pixmapFocalPoint in a way
 * that doesn't cause image movement doesn't require calling this method.
 */
void ImageViewBase::updateWidgetTransform() {
  const TransformChangeWatcher watcher(*this);

  const QRectF virt_rect(virtualDisplayRect());
  const QPointF virt_origin(m_imageToVirtual.map(m_pixmapFocalPoint));
  const QPointF widget_origin(m_widgetFocalPoint);

  QSizeF zoom1_widget_size(virt_rect.size());
  zoom1_widget_size.scale(maxViewportRect().size(), Qt::KeepAspectRatio);

  const double zoom1_x = zoom1_widget_size.width() / virt_rect.width();
  const double zoom1_y = zoom1_widget_size.height() / virt_rect.height();

  QTransform xform;
  xform.translate(-virt_origin.x(), -virt_origin.y());
  xform *= QTransform().scale(zoom1_x * m_zoom, zoom1_y * m_zoom);
  xform *= QTransform().translate(widget_origin.x(), widget_origin.y());

  m_virtualToWidget = xform;
  m_widgetToVirtual = m_virtualToWidget.inverted();
}

/**
 * Updates m_virtualToWidget and m_widgetToVirtual and adjusts
 * the focal point if necessary.\n
 * To be called whenever m_imageToVirt is modified in such a way that
 * may invalidate the focal point.
 */
void ImageViewBase::updateWidgetTransformAndFixFocalPoint(const FocalPointMode mode) {
  const TransformChangeWatcher watcher(*this);

  // This must go before getIdealWidgetFocalPoint(), as it
  // recalculates m_virtualToWidget, that is used by
  // getIdealWidgetFocalPoint().
  updateWidgetTransform();

  const QPointF ideal_widget_fp(getIdealWidgetFocalPoint(mode));
  if (ideal_widget_fp != m_widgetFocalPoint) {
    m_widgetFocalPoint = ideal_widget_fp;
    updateWidgetTransform();
  }
}

/**
 * Returns a proposed value for m_widgetFocalPoint to minimize the
 * unused widget space.  Unused widget space indicates one or both
 * of the following:
 * \li The image is smaller than the display area.
 * \li Parts of the image are outside of the display area.
 *
 * \param mode If set to CENTER_IF_FITS, then the returned focal point
 *        will center the image if it completely fits into the widget.
 *        This works in horizontal and vertical directions independently.\n
 *        If \p mode is set to DONT_CENTER and the image completely fits
 *        the widget, then the returned focal point will cause a minimal
 *        move to force the whole image to be visible.
 *
 * In case there is no unused widget space, the returned focal point
 * is equal to the current focal point (m_widgetFocalPoint).  This works
 * in horizontal and vertical dimensions independently.
 */
QPointF ImageViewBase::getIdealWidgetFocalPoint(const FocalPointMode mode) const {
  // Widget rect reduced by margins.
  const QRectF display_area(maxViewportRect());

  // The virtual image rectangle in widget coordinates.
  const QRectF image_area(m_virtualToWidget.mapRect(virtualDisplayRect()));
  // Unused display space from each side.
  const double left_margin = image_area.left() - display_area.left();
  const double right_margin = display_area.right() - image_area.right();
  const double top_margin = image_area.top() - display_area.top();
  const double bottom_margin = display_area.bottom() - image_area.bottom();

  QPointF widget_focal_point(m_widgetFocalPoint);

  if ((mode == CENTER_IF_FITS) && (left_margin + right_margin >= 0.0)) {
    // Image fits horizontally, so center it in that direction
    // by equalizing its left and right margins.
    const double new_margins = 0.5 * (left_margin + right_margin);
    widget_focal_point.rx() += new_margins - left_margin;
  } else if ((left_margin < 0.0) && (right_margin > 0.0)) {
    // Move image to the right so that either left_margin or
    // right_margin becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(left_margin), std::fabs(right_margin));
    widget_focal_point.rx() += movement;
  } else if ((right_margin < 0.0) && (left_margin > 0.0)) {
    // Move image to the left so that either left_margin or
    // right_margin becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(left_margin), std::fabs(right_margin));
    widget_focal_point.rx() -= movement;
  }

  if ((mode == CENTER_IF_FITS) && (top_margin + bottom_margin >= 0.0)) {
    // Image fits vertically, so center it in that direction
    // by equalizing its top and bottom margins.
    const double new_margins = 0.5 * (top_margin + bottom_margin);
    widget_focal_point.ry() += new_margins - top_margin;
  } else if ((top_margin < 0.0) && (bottom_margin > 0.0)) {
    // Move image down so that either top_margin or bottom_margin
    // becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(top_margin), std::fabs(bottom_margin));
    widget_focal_point.ry() += movement;
  } else if ((bottom_margin < 0.0) && (top_margin > 0.0)) {
    // Move image up so that either top_margin or bottom_margin
    // becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(top_margin), std::fabs(bottom_margin));
    widget_focal_point.ry() -= movement;
  }

  return widget_focal_point;
}  // ImageViewBase::getIdealWidgetFocalPoint

void ImageViewBase::setNewWidgetFP(const QPointF widget_fp, const bool update) {
  if (widget_fp != m_widgetFocalPoint) {
    m_widgetFocalPoint = widget_fp;
    updateWidgetTransform();
    if (update) {
      this->update();
    }
  }
}

/**
 * Used when dragging the image.  It adjusts the movement to disallow
 * dragging it away from the ideal position (determined by
 * getIdealWidgetFocalPoint()).  Movement towards the ideal position
 * is permitted.  This works independently in horizontal and vertical
 * direction.
 *
 * \param proposed_widget_fp The proposed value for m_widgetFocalPoint.
 * \param update Whether to call this->update() in case the focal point
 *        has changed.
 */
void ImageViewBase::adjustAndSetNewWidgetFP(const QPointF proposed_widget_fp, const bool update) {
  // We first apply the proposed focal point, and only then
  // calculate the ideal one.  That's done because
  // the ideal focal point is the current focal point when
  // no widget space is wasted (image covers the whole widget).
  // We don't want the ideal focal point to be equal to the current
  // one, as that would disallow any movements.
  const QPointF old_widget_fp(m_widgetFocalPoint);
  setNewWidgetFP(proposed_widget_fp, update);

  const QPointF ideal_widget_fp(getIdealWidgetFocalPoint(CENTER_IF_FITS));

  const QPointF towards_ideal(ideal_widget_fp - old_widget_fp);
  const QPointF towards_proposed(proposed_widget_fp - old_widget_fp);

  QPointF movement(towards_proposed);

  // Horizontal movement.
  if (towards_ideal.x() * towards_proposed.x() < 0.0) {
    // Wrong direction - no movement at all.
    movement.setX(0.0);
  } else if (std::fabs(towards_proposed.x()) > std::fabs(towards_ideal.x())) {
    // Too much movement - limit it.
    movement.setX(towards_ideal.x());
  }
  // Vertical movement.
  if (towards_ideal.y() * towards_proposed.y() < 0.0) {
    // Wrong direction - no movement at all.
    movement.setY(0.0);
  } else if (std::fabs(towards_proposed.y()) > std::fabs(towards_ideal.y())) {
    // Too much movement - limit it.
    movement.setY(towards_ideal.y());
  }

  const QPointF adjusted_widget_fp(old_widget_fp + movement);
  if (adjusted_widget_fp != m_widgetFocalPoint) {
    m_widgetFocalPoint = adjusted_widget_fp;
    updateWidgetTransform();
    if (update) {
      this->update();
    }
  }
}  // ImageViewBase::adjustAndSetNewWidgetFP

/**
 * Returns the center point of the available display area.
 */
QPointF ImageViewBase::centeredWidgetFocalPoint() const {
  return maxViewportRect().center();
}

void ImageViewBase::setWidgetFocalPointWithoutMoving(const QPointF new_widget_fp) {
  m_widgetFocalPoint = new_widget_fp;
  m_pixmapFocalPoint = m_virtualToImage.map(m_widgetToVirtual.map(m_widgetFocalPoint));
}

/**
 * Returns true if m_hqPixmap is valid and up to date.
 */
bool ImageViewBase::validateHqPixmap() const {
  if (!m_hqTransformEnabled) {
    return false;
  }

  if (m_hqPixmap.isNull()) {
    return false;
  }

  if (m_hqSourceId != m_image.cacheKey()) {
    return false;
  }

  if (m_hqXform != m_imageToVirtual * m_virtualToWidget) {
    return false;
  }

  return true;
}

void ImageViewBase::scheduleHqVersionRebuild() {
  const QTransform xform(m_imageToVirtual * m_virtualToWidget);

  if (!m_timer.isActive() || (m_potentialHqXform != xform)) {
    if (m_ptrHqTransformTask) {
      m_ptrHqTransformTask->cancel();
      m_ptrHqTransformTask.reset();
    }
    m_potentialHqXform = xform;
  }
  m_timer.start();
}

void ImageViewBase::initiateBuildingHqVersion() {
  if (validateHqPixmap()) {
    return;
  }

  m_hqPixmap = QPixmap();

  if (m_ptrHqTransformTask) {
    m_ptrHqTransformTask->cancel();
    m_ptrHqTransformTask.reset();
  }

  const QTransform xform(m_imageToVirtual * m_virtualToWidget);
  const auto task = make_intrusive<HqTransformTask>(this, m_image, xform, viewport()->size());

  backgroundExecutor().enqueueTask(task);

  m_ptrHqTransformTask = task;
  m_hqXform = xform;
  m_hqSourceId = m_image.cacheKey();
}

/**
 * Gets called from HqTransformationTask::Result.
 */
void ImageViewBase::hqVersionBuilt(const QPoint& origin, const QImage& image) {
  if (!m_hqTransformEnabled) {
    return;
  }

  m_hqPixmap = QPixmap::fromImage(image);
  m_hqPixmapPos = origin;
  m_ptrHqTransformTask.reset();
  update();
}

void ImageViewBase::updateStatusTipAndCursor() {
  updateStatusTip();
  updateCursor();
}

void ImageViewBase::updateStatusTip() {
  ensureStatusTip(m_interactionState.statusTip());
}

void ImageViewBase::updateCursor() {
  viewport()->setCursor(m_interactionState.cursor());
}

void ImageViewBase::maybeQueueRedraw() {
  if (m_interactionState.redrawRequested()) {
    m_interactionState.setRedrawRequested(false);
    update();
  }
}

BackgroundExecutor& ImageViewBase::backgroundExecutor() {
  static BackgroundExecutor executor;

  return executor;
}

void ImageViewBase::updateCursorPos(const QPointF& pos) {
  if (pos != m_cursorPos) {
    m_cursorPos = pos;
    if (!m_cursorTrackerTimer.isActive()) {
      // report cursor pos once in 150 msec
      m_cursorTrackerTimer.start(150);
    }
  }
}

void ImageViewBase::updatePhysSize() {
  m_infoProvider.setPhysSize(m_virtualImageCropArea.boundingRect().size());
}

ImageViewInfoProvider& ImageViewBase::infoProvider() {
  return m_infoProvider;
}

/*==================== ImageViewBase::HqTransformTask ======================*/

ImageViewBase::HqTransformTask::HqTransformTask(ImageViewBase* image_view,
                                                const QImage& image,
                                                const QTransform& xform,
                                                const QSize& target_size)
    : m_ptrResult(new Result(image_view)), m_image(image), m_xform(xform), m_targetSize(target_size) {}

intrusive_ptr<AbstractCommand<void>> ImageViewBase::HqTransformTask::operator()() {
  if (isCancelled()) {
    return nullptr;
  }

  const QRect target_rect(
      m_xform.map(QRectF(m_image.rect())).boundingRect().toRect().intersected(QRect(QPoint(0, 0), m_targetSize)));

  QImage hq_image(
      transform(m_image, m_xform, target_rect, OutsidePixels::assumeWeakColor(Qt::white), QSizeF(0.0, 0.0)));

  // In many cases m_image and therefore hq_image are grayscale with
  // a palette, but given that hq_image will be converted to a QPixmap
  // on the GUI thread, it's better to convert it to RGB as a preparation
  // step while we are still in a background thread.
  hq_image = hq_image.convertToFormat(hq_image.hasAlphaChannel() ? QImage::Format_ARGB32_Premultiplied
                                                                 : QImage::Format_RGB32);

  m_ptrResult->setData(target_rect.topLeft(), hq_image);

  return m_ptrResult;
}

/*================ ImageViewBase::HqTransformTask::Result ================*/

ImageViewBase::HqTransformTask::Result::Result(ImageViewBase* image_view) : m_ptrImageView(image_view) {}

void ImageViewBase::HqTransformTask::Result::setData(const QPoint& origin, const QImage& hq_image) {
  m_hqImage = hq_image;
  m_origin = origin;
}

void ImageViewBase::HqTransformTask::Result::operator()() {
  if (m_ptrImageView && !isCancelled()) {
    m_ptrImageView->hqVersionBuilt(m_origin, m_hqImage);
  }
}

/*================= ImageViewBase::TempFocalPointAdjuster =================*/

ImageViewBase::TempFocalPointAdjuster::TempFocalPointAdjuster(ImageViewBase& obj)
    : m_rObj(obj), m_origWidgetFP(obj.getWidgetFocalPoint()) {
  obj.setWidgetFocalPointWithoutMoving(obj.centeredWidgetFocalPoint());
}

ImageViewBase::TempFocalPointAdjuster::TempFocalPointAdjuster(ImageViewBase& obj, const QPointF temp_widget_fp)
    : m_rObj(obj), m_origWidgetFP(obj.getWidgetFocalPoint()) {
  obj.setWidgetFocalPointWithoutMoving(temp_widget_fp);
}

ImageViewBase::TempFocalPointAdjuster::~TempFocalPointAdjuster() {
  m_rObj.setWidgetFocalPointWithoutMoving(m_origWidgetFP);
}

/*================== ImageViewBase::TransformChangeWatcher ================*/

ImageViewBase::TransformChangeWatcher::TransformChangeWatcher(ImageViewBase& owner)
    : m_rOwner(owner),
      m_imageToVirtual(owner.m_imageToVirtual),
      m_virtualToWidget(owner.m_virtualToWidget),
      m_virtualDisplayArea(owner.m_virtualDisplayArea) {
  ++m_rOwner.m_transformChangeWatchersActive;
}

ImageViewBase::TransformChangeWatcher::~TransformChangeWatcher() {
  if (--m_rOwner.m_transformChangeWatchersActive == 0) {
    if ((m_imageToVirtual != m_rOwner.m_imageToVirtual) || (m_virtualToWidget != m_rOwner.m_virtualToWidget)
        || (m_virtualDisplayArea != m_rOwner.m_virtualDisplayArea)) {
      m_rOwner.transformChanged();
    }
  }
}
