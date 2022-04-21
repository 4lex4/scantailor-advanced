// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageViewBase.h"

#include <PolygonUtils.h>
#include <Transform.h>

#include <QApplication>
#if QT_VERSION_MAJOR == 5
#include <QGLWidget>
#define QOpenGLWidget QGLWidget
#else
#include <QtOpenGLWidgets/QOpenGLWidget>
#endif
#include <QMouseEvent>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QScrollBar>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>

#include "ApplicationSettings.h"
#include "BackgroundExecutor.h"
#include "ColorSchemeManager.h"
#include "Dpm.h"
#include "ImagePresentation.h"
#include "OpenGLSupport.h"
#include "PixmapRenderer.h"
#include "ScopedIncDec.h"
#include "UnitsProvider.h"
#include "Utils.h"

using namespace core;
using namespace imageproc;

class ImageViewBase::HqTransformTask : public AbstractCommand<std::shared_ptr<AbstractCommand<void>>>, public QObject {
  DECLARE_NON_COPYABLE(HqTransformTask)

 public:
  HqTransformTask(ImageViewBase* imageView, const QImage& image, const QTransform& xform, const QSize& targetSize);

  void cancel() { m_result->cancel(); }

  bool isCancelled() const { return m_result->isCancelled(); }

  std::shared_ptr<AbstractCommand<void>> operator()() override;

 private:
  class Result : public AbstractCommand<void> {
   public:
    explicit Result(ImageViewBase* imageView);

    void setData(const QPoint& origin, const QImage& hqImage);

    void cancel() { m_cancelFlag.fetchAndStoreRelaxed(1); }

    bool isCancelled() const { return m_cancelFlag.fetchAndAddRelaxed(0) != 0; }

    void operator()() override;

   private:
    QPointer<ImageViewBase> m_imageView;
    QPoint m_origin;
    QImage m_hqImage;
    mutable QAtomicInt m_cancelFlag;
  };


  std::shared_ptr<Result> m_result;
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
   * Change the widget focal point to \p tempWidgetFp
   */
  TempFocalPointAdjuster(ImageViewBase& obj, QPointF tempWidgetFp);

  /**
   * Restore the widget focal point.
   */
  ~TempFocalPointAdjuster();

 private:
  ImageViewBase& m_obj;
  QPointF m_origWidgetFP;
};


class ImageViewBase::TransformChangeWatcher {
 public:
  explicit TransformChangeWatcher(ImageViewBase& owner);

  ~TransformChangeWatcher();

 private:
  ImageViewBase& m_owner;
  QTransform m_imageToVirtual;
  QTransform m_virtualToWidget;
  QRectF m_virtualDisplayArea;
};


ImageViewBase::ImageViewBase(const QImage& image,
                             const ImagePixmapUnion& downscaledVersion,
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
   * a color different from QPalette::Window at the first show on Windows.
   * Here we make it not fill it automatically at all
   * doing the work in paintEvent().
   */
  setAttribute(Qt::WA_OpaquePaintEvent);

  /* For some reason, the default viewport fills background with
   * a color different from QPalette::Window. Here we make it not
   * fill it at all, assuming QMainWindow will do that anyway
   * (with the correct color). Note that an attempt to do the same
   * to an OpenGL viewport produces "black hole" artefacts. Therefore,
   * we do this before setting an OpenGL viewport rather than after.
   */
  viewport()->setAutoFillBackground(false);

  if (ApplicationSettings::getInstance().isOpenGlEnabled()) {
    if (OpenGLSupport::supported()) {
      setViewport(new QOpenGLWidget());
    }
  }

  setFrameShape(QFrame::NoFrame);
  viewport()->setFocusPolicy(Qt::WheelFocus);

  if (downscaledVersion.isNull()) {
    m_pixmap = QPixmap::fromImage(createDownscaledImage(image));
  } else if (downscaledVersion.pixmap().isNull()) {
    m_pixmap = QPixmap::fromImage(downscaledVersion.image());
  } else {
    m_pixmap = downscaledVersion.pixmap();
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
    if (m_hqTransformTask) {
      m_hqTransformTask->cancel();
      m_hqTransformTask.reset();
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
  const Dpm oDpm(image);
  const Dpm dDpm(Dpi(200, 200));

  const int oW = image.width();
  const int oH = image.height();

  int dW = oW * dDpm.horizontal() / oDpm.horizontal();
  int dH = oH * dDpm.vertical() / oDpm.vertical();
  dW = qBound(1, dW, oW);
  dH = qBound(1, dH, oH);

  if ((dW * 1.2 > oW) || (dH * 1.2 > oH)) {
    // Sizes are close - no point in downscaling.
    return image;
  }

  QTransform xform;
  xform.scale((double) dW / oW, (double) dH / oH);
  return transform(image, xform, QRect(0, 0, dW, dH), OutsidePixels::assumeColor(Qt::white));
}

QRectF ImageViewBase::maxViewportRect() const {
  const QRectF viewportRect(QPointF(0, 0), maximumViewportSize());
  QRectF r(viewportRect);
  r.adjust(m_margins.left(), m_margins.top(), -m_margins.right(), -m_margins.bottom());
  if (r.isEmpty()) {
    return QRectF(viewportRect.center(), viewportRect.center());
  }
  return r;
}

QRectF ImageViewBase::dynamicViewportRect() const {
  const QRectF viewportRect(viewport()->rect());
  QRectF r(viewportRect);
  r.adjust(m_margins.left(), m_margins.top(), -m_margins.right(), -m_margins.bottom());
  if (r.isEmpty()) {
    return QRectF(viewportRect.center(), viewportRect.center());
  }
  return r;
}

QRectF ImageViewBase::getOccupiedWidgetRect() const {
  const QRectF widgetRect(m_virtualToWidget.mapRect(virtualDisplayRect()));
  return widgetRect.intersected(dynamicViewportRect());
}

void ImageViewBase::setWidgetFocalPoint(const QPointF& widgetFp) {
  setNewWidgetFP(widgetFp, /*update =*/true);
}

void ImageViewBase::adjustAndSetWidgetFocalPoint(const QPointF& widgetFp) {
  adjustAndSetNewWidgetFP(widgetFp, /*update=*/true);
}

void ImageViewBase::setZoomLevel(double zoom) {
  if (m_zoom != zoom) {
    m_zoom = zoom;
    updateWidgetTransform();
    update();
  }
}

void ImageViewBase::moveTowardsIdealPosition(const double pixelLength) {
  if (pixelLength <= 0) {
    // The name implies we are moving *towards* the ideal position.
    return;
  }

  const QPointF idealWidgetFp(getIdealWidgetFocalPoint(CENTER_IF_FITS));
  if (idealWidgetFp == m_widgetFocalPoint) {
    return;
  }

  QPointF vec(idealWidgetFp - m_widgetFocalPoint);
  const double maxLength = std::sqrt(vec.x() * vec.x() + vec.y() * vec.y());
  if (pixelLength >= maxLength) {
    m_widgetFocalPoint = idealWidgetFp;
  } else {
    vec *= pixelLength / maxLength;
    m_widgetFocalPoint += vec;
  }

  updateWidgetTransform();
  update();
}

void ImageViewBase::updateTransform(const ImagePresentation& presentation) {
  const TransformChangeWatcher watcher(*this);
  const TempFocalPointAdjuster tempFp(*this);

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
  const TempFocalPointAdjuster tempFp(*this);

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
  const TempFocalPointAdjuster tempFp(*this);

  // An arbitrary line in image coordinates.
  const QLineF imageLine(0.0, 0.0, 1.0, 1.0);

  const QLineF widgetLineBefore((m_imageToVirtual * m_virtualToWidget).map(imageLine));

  m_imageToVirtual = presentation.transform();
  m_virtualToImage = m_imageToVirtual.inverted();
  m_virtualImageCropArea = presentation.cropArea();
  m_virtualDisplayArea = presentation.displayArea();

  updateWidgetTransform();

  const QLineF widgetLineAfter((m_imageToVirtual * m_virtualToWidget).map(imageLine));

  m_zoom *= widgetLineBefore.length() / widgetLineAfter.length();
  updateWidgetTransform();

  update();
  updatePhysSize();
}

void ImageViewBase::ensureStatusTip(const QString& statusTip) {
  const QString curStatusTip(this->statusTip());
  if (curStatusTip.constData() == statusTip.constData()) {
    return;
  }
  if (curStatusTip == statusTip) {
    return;
  }

  viewport()->setStatusTip(statusTip);

  if (viewport()->underMouse()) {
    // Note that setStatusTip() alone is not enough,
    // as it's only taken into account when the mouse
    // enters the widget.
    // Also note that we use postEvent() rather than sendEvent(),
    // because sendEvent() may immediately process other events.
    QApplication::postEvent(viewport(), new QStatusTipEvent(statusTip));
  }
}

void ImageViewBase::paintEvent(QPaintEvent* event) {
  QPainter painter(viewport());

  // Fill the background as Qt::WA_OpaquePaintEvent attribute is enabled.
  painter.fillRect(viewport()->rect(), palette().color(backgroundRole()));

  painter.save();

  const double xscale = m_virtualToWidget.m11();

  // Width of a source pixel in mm, as it's displayed on screen.
  const double pixelWidth = widthMM() * xscale / width();

  // Make clipping smooth.
  painter.setRenderHint(QPainter::Antialiasing, true);

  // Disable antialiasing for large zoom levels.
  painter.setRenderHint(QPainter::SmoothPixmapTransform, pixelWidth < 0.5);

  if (validateHqPixmap()) {
    // HQ pixmap maps one to one to screen pixels, so antialiasing is not necessary.
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    QPainterPath clipPath;
    clipPath.addPolygon(m_virtualToWidget.map(m_virtualImageCropArea));
    painter.setClipPath(clipPath);

    painter.drawPixmap(m_hqPixmapPos, m_hqPixmap);
  } else {
    scheduleHqVersionRebuild();

    const QTransform pixmapToVirtual(m_pixmapToImage * m_imageToVirtual);
    painter.setWorldTransform(pixmapToVirtual * m_virtualToWidget);

    QPainterPath clipPath;
    clipPath.addPolygon(pixmapToVirtual.inverted().map(m_virtualImageCropArea));
    painter.setClipPath(clipPath);

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
#if QT_VERSION_MAJOR == 5
  updateCursorPos(event->localPos());
#else
  updateCursorPos(event->position());
#endif
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
    const TempFocalPointAdjuster tempFp(*this, QPointF(0, 0));
    updateTransformPreservingScale(ImagePresentation(m_imageToVirtual, m_virtualImageCropArea, m_virtualDisplayArea));
  }
}

#if QT_VERSION_MAJOR == 5
void ImageViewBase::enterEvent(QEvent* event) {
#else
void ImageViewBase::enterEvent(QEnterEvent* event) {
#endif
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
    if (auto* infoListener = Utils::castOrFindChild<ImageViewInfoListener*>(mainWindow->statusBar())) {
      infoProvider().addListener(infoListener);
    }
  }
}

void ImageViewBase::hideEvent(QHideEvent* event) {
  infoProvider().removeAllListeners();
  QWidget::hideEvent(event);
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
  const QPointF viewportCenter(maxViewportRect().center());
  const QPointF pictureCenter(picture.center());
  QRectF viewport(maxViewportRect());

  // Introduction of one scrollbar will decrease the available size in
  // another direction, which may cause a scrollbar in that direction
  // to become necessary.  For this reason, we have a loop here.
  for (int i = 0; i < 2; ++i) {
    const double xval = pictureCenter.x();
    double xmin, xmax;  // Minimum and maximum positions for picture center.
    if (pictureCenter.x() < viewportCenter.x()) {
      xmin = std::min<double>(xval, viewport.right() - 0.5 * picture.width());
      xmax = std::max<double>(viewportCenter.x(), viewport.left() + 0.5 * picture.width());
    } else {
      xmax = std::max<double>(xval, viewport.left() + 0.5 * picture.width());
      xmin = std::min<double>(viewportCenter.x(), viewport.right() - 0.5 * picture.width());
    }

    const double yval = pictureCenter.y();
    double ymin, ymax;  // Minimum and maximum positions for picture center.
    if (pictureCenter.y() < viewportCenter.y()) {
      ymin = std::min<double>(yval, viewport.bottom() - 0.5 * picture.height());
      ymax = std::max<double>(viewportCenter.y(), viewport.top() + 0.5 * picture.height());
    } else {
      ymax = std::max<double>(yval, viewport.top() + 0.5 * picture.height());
      ymin = std::min<double>(viewportCenter.y(), viewport.bottom() - 0.5 * picture.height());
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

    const QRectF oldViewport(viewport);
    viewport = dynamicViewportRect();
    if (viewport == oldViewport) {
      break;
    }
  }
}  // ImageViewBase::updateScrollBars

void ImageViewBase::reactToScrollBars() {
  if (m_ignoreScrollEvents) {
    return;
  }

  const TransformChangeWatcher watcher(*this);

  const QPointF rawPosition(horizontalScrollBar()->value(), verticalScrollBar()->value());
  const QPointF newFp(m_scrollTransform.map(rawPosition));
  const QPointF oldFp(getWidgetFocalPoint());

  m_pixmapFocalPoint = m_virtualToImage.map(m_virtualDisplayArea.center());
  m_widgetFocalPoint = newFp;
  updateWidgetTransform();

  setWidgetFocalPointWithoutMoving(oldFp);
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

  const QRectF virtRect(virtualDisplayRect());
  const QPointF virtOrigin(m_imageToVirtual.map(m_pixmapFocalPoint));
  const QPointF widgetOrigin(m_widgetFocalPoint);

  QSizeF zoom1WidgetSize(virtRect.size());
  zoom1WidgetSize.scale(maxViewportRect().size(), Qt::KeepAspectRatio);

  const double zoom1X = zoom1WidgetSize.width() / virtRect.width();
  const double zoom1Y = zoom1WidgetSize.height() / virtRect.height();

  QTransform xform;
  xform.translate(-virtOrigin.x(), -virtOrigin.y());
  xform *= QTransform().scale(zoom1X * m_zoom, zoom1Y * m_zoom);
  xform *= QTransform().translate(widgetOrigin.x(), widgetOrigin.y());

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

  const QPointF idealWidgetFp(getIdealWidgetFocalPoint(mode));
  if (idealWidgetFp != m_widgetFocalPoint) {
    m_widgetFocalPoint = idealWidgetFp;
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
  const QRectF displayArea(maxViewportRect());

  // The virtual image rectangle in widget coordinates.
  const QRectF imageArea(m_virtualToWidget.mapRect(virtualDisplayRect()));
  // Unused display space from each side.
  const double leftMargin = imageArea.left() - displayArea.left();
  const double rightMargin = displayArea.right() - imageArea.right();
  const double topMargin = imageArea.top() - displayArea.top();
  const double bottomMargin = displayArea.bottom() - imageArea.bottom();

  QPointF widgetFocalPoint(m_widgetFocalPoint);

  if ((mode == CENTER_IF_FITS) && (leftMargin + rightMargin >= 0.0)) {
    // Image fits horizontally, so center it in that direction
    // by equalizing its left and right margins.
    const double newMargins = 0.5 * (leftMargin + rightMargin);
    widgetFocalPoint.rx() += newMargins - leftMargin;
  } else if ((leftMargin < 0.0) && (rightMargin > 0.0)) {
    // Move image to the right so that either leftMargin or
    // rightMargin becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(leftMargin), std::fabs(rightMargin));
    widgetFocalPoint.rx() += movement;
  } else if ((rightMargin < 0.0) && (leftMargin > 0.0)) {
    // Move image to the left so that either leftMargin or
    // rightMargin becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(leftMargin), std::fabs(rightMargin));
    widgetFocalPoint.rx() -= movement;
  }

  if ((mode == CENTER_IF_FITS) && (topMargin + bottomMargin >= 0.0)) {
    // Image fits vertically, so center it in that direction
    // by equalizing its top and bottom margins.
    const double newMargins = 0.5 * (topMargin + bottomMargin);
    widgetFocalPoint.ry() += newMargins - topMargin;
  } else if ((topMargin < 0.0) && (bottomMargin > 0.0)) {
    // Move image down so that either topMargin or bottomMargin
    // becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(topMargin), std::fabs(bottomMargin));
    widgetFocalPoint.ry() += movement;
  } else if ((bottomMargin < 0.0) && (topMargin > 0.0)) {
    // Move image up so that either topMargin or bottomMargin
    // becomes zero, whichever requires less movement.
    const double movement = std::min(std::fabs(topMargin), std::fabs(bottomMargin));
    widgetFocalPoint.ry() -= movement;
  }
  return widgetFocalPoint;
}  // ImageViewBase::getIdealWidgetFocalPoint

void ImageViewBase::setNewWidgetFP(const QPointF widgetFp, const bool update) {
  if (widgetFp != m_widgetFocalPoint) {
    m_widgetFocalPoint = widgetFp;
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
 * \param proposedWidgetFp The proposed value for m_widgetFocalPoint.
 * \param update Whether to call this->update() in case the focal point
 *        has changed.
 */
void ImageViewBase::adjustAndSetNewWidgetFP(const QPointF proposedWidgetFp, const bool update) {
  // We first apply the proposed focal point, and only then
  // calculate the ideal one.  That's done because
  // the ideal focal point is the current focal point when
  // no widget space is wasted (image covers the whole widget).
  // We don't want the ideal focal point to be equal to the current
  // one, as that would disallow any movements.
  const QPointF oldWidgetFp(m_widgetFocalPoint);
  setNewWidgetFP(proposedWidgetFp, update);

  const QPointF idealWidgetFp(getIdealWidgetFocalPoint(CENTER_IF_FITS));

  const QPointF towardsIdeal(idealWidgetFp - oldWidgetFp);
  const QPointF towardsProposed(proposedWidgetFp - oldWidgetFp);

  QPointF movement(towardsProposed);

  // Horizontal movement.
  if (towardsIdeal.x() * towardsProposed.x() < 0.0) {
    // Wrong direction - no movement at all.
    movement.setX(0.0);
  } else if (std::fabs(towardsProposed.x()) > std::fabs(towardsIdeal.x())) {
    // Too much movement - limit it.
    movement.setX(towardsIdeal.x());
  }
  // Vertical movement.
  if (towardsIdeal.y() * towardsProposed.y() < 0.0) {
    // Wrong direction - no movement at all.
    movement.setY(0.0);
  } else if (std::fabs(towardsProposed.y()) > std::fabs(towardsIdeal.y())) {
    // Too much movement - limit it.
    movement.setY(towardsIdeal.y());
  }

  const QPointF adjustedWidgetFp(oldWidgetFp + movement);
  if (adjustedWidgetFp != m_widgetFocalPoint) {
    m_widgetFocalPoint = adjustedWidgetFp;
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

void ImageViewBase::setWidgetFocalPointWithoutMoving(const QPointF newWidgetFp) {
  m_widgetFocalPoint = newWidgetFp;
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
    if (m_hqTransformTask) {
      m_hqTransformTask->cancel();
      m_hqTransformTask.reset();
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

  if (m_hqTransformTask) {
    m_hqTransformTask->cancel();
    m_hqTransformTask.reset();
  }

  const QTransform xform(m_imageToVirtual * m_virtualToWidget);
  const auto task = std::make_shared<HqTransformTask>(this, m_image, xform, viewport()->size());

  backgroundExecutor().enqueueTask(task);

  m_hqTransformTask = task;
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
  m_hqTransformTask.reset();
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

ImageViewBase::HqTransformTask::HqTransformTask(ImageViewBase* imageView,
                                                const QImage& image,
                                                const QTransform& xform,
                                                const QSize& targetSize)
    : m_result(std::make_shared<Result>(imageView)), m_image(image), m_xform(xform), m_targetSize(targetSize) {}

std::shared_ptr<AbstractCommand<void>> ImageViewBase::HqTransformTask::operator()() {
  if (isCancelled()) {
    return nullptr;
  }

  const QRect targetRect(
      m_xform.map(QRectF(m_image.rect())).boundingRect().toRect().intersected(QRect(QPoint(0, 0), m_targetSize)));

  QImage hqImage(transform(m_image, m_xform, targetRect, OutsidePixels::assumeWeakColor(Qt::white), QSizeF(0.0, 0.0)));

  // In many cases m_image and therefore hqImage are grayscale with
  // a palette, but given that hqImage will be converted to a QPixmap
  // on the GUI thread, it's better to convert it to RGB as a preparation
  // step while we are still in a background thread.
  hqImage
      = hqImage.convertToFormat(hqImage.hasAlphaChannel() ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);

  m_result->setData(targetRect.topLeft(), hqImage);
  return m_result;
}

/*================ ImageViewBase::HqTransformTask::Result ================*/

ImageViewBase::HqTransformTask::Result::Result(ImageViewBase* imageView) : m_imageView(imageView) {}

void ImageViewBase::HqTransformTask::Result::setData(const QPoint& origin, const QImage& hqImage) {
  m_hqImage = hqImage;
  m_origin = origin;
}

void ImageViewBase::HqTransformTask::Result::operator()() {
  if (m_imageView && !isCancelled()) {
    m_imageView->hqVersionBuilt(m_origin, m_hqImage);
  }
}

/*================= ImageViewBase::TempFocalPointAdjuster =================*/

ImageViewBase::TempFocalPointAdjuster::TempFocalPointAdjuster(ImageViewBase& obj)
    : m_obj(obj), m_origWidgetFP(obj.getWidgetFocalPoint()) {
  obj.setWidgetFocalPointWithoutMoving(obj.centeredWidgetFocalPoint());
}

ImageViewBase::TempFocalPointAdjuster::TempFocalPointAdjuster(ImageViewBase& obj, const QPointF tempWidgetFp)
    : m_obj(obj), m_origWidgetFP(obj.getWidgetFocalPoint()) {
  obj.setWidgetFocalPointWithoutMoving(tempWidgetFp);
}

ImageViewBase::TempFocalPointAdjuster::~TempFocalPointAdjuster() {
  m_obj.setWidgetFocalPointWithoutMoving(m_origWidgetFP);
}

/*================== ImageViewBase::TransformChangeWatcher ================*/

ImageViewBase::TransformChangeWatcher::TransformChangeWatcher(ImageViewBase& owner)
    : m_owner(owner),
      m_imageToVirtual(owner.m_imageToVirtual),
      m_virtualToWidget(owner.m_virtualToWidget),
      m_virtualDisplayArea(owner.m_virtualDisplayArea) {
  ++m_owner.m_transformChangeWatchersActive;
}

ImageViewBase::TransformChangeWatcher::~TransformChangeWatcher() {
  if (--m_owner.m_transformChangeWatchersActive == 0) {
    if ((m_imageToVirtual != m_owner.m_imageToVirtual) || (m_virtualToWidget != m_owner.m_virtualToWidget)
        || (m_virtualDisplayArea != m_owner.m_virtualDisplayArea)) {
      m_owner.transformChanged();
    }
  }
}
