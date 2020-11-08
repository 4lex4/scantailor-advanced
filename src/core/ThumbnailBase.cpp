// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ThumbnailBase.h"

#include <PolygonUtils.h>

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QStyleOptionGraphicsItem>
#include <cmath>
#include <utility>

#include "ApplicationSettings.h"
#include "PixmapRenderer.h"

using namespace imageproc;

class ThumbnailBase::LoadCompletionHandler : public AbstractCommand<void, const ThumbnailLoadResult&> {
  DECLARE_NON_COPYABLE(LoadCompletionHandler)

 public:
  explicit LoadCompletionHandler(ThumbnailBase* thumb) : m_thumb(thumb) {}

  void operator()(const ThumbnailLoadResult& result) override { m_thumb->handleLoadResult(result); }

 private:
  ThumbnailBase* m_thumb;
};


ThumbnailBase::ThumbnailBase(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
                             const QSizeF& maxSize,
                             const ImageId& imageId,
                             const ImageTransformation& imageXform)
    : ThumbnailBase(std::move(thumbnailCache), maxSize, imageId, imageXform, imageXform.resultingRect()) {}

ThumbnailBase::ThumbnailBase(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
                             const QSizeF& maxSize,
                             const ImageId& imageId,
                             const ImageTransformation& imageXform,
                             QRectF displayArea)
    : m_thumbnailCache(std::move(thumbnailCache)),
      m_maxSize(maxSize),
      m_imageId(imageId),
      m_imageXform(imageXform),
      m_displayArea(displayArea) {
  setImageXform(m_imageXform);
}

ThumbnailBase::~ThumbnailBase() = default;

QRectF ThumbnailBase::boundingRect() const {
  return m_boundingRect;
}

void ThumbnailBase::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  QPixmap pixmap;

  if (!m_completionHandler) {
    auto handler = std::make_shared<LoadCompletionHandler>(this);
    const ThumbnailPixmapCache::Status status = m_thumbnailCache->loadRequest(m_imageId, pixmap, handler);
    if (status == ThumbnailPixmapCache::QUEUED) {
      m_completionHandler.swap(handler);
    }
  }

  const QTransform thumbToDisplay = painter->worldTransform();
  const QTransform imageToDisplay = m_postScaleXform * thumbToDisplay;

  if (pixmap.isNull()) {
    const double border = 1.0;
    const double shadow = 2.0;
    QRectF rect(m_boundingRect);
    rect.adjust(border, border, -(border + shadow), -(border + shadow));

    painter->fillRect(m_boundingRect, QColor(0x00, 0x00, 0x00));
    painter->fillRect(rect, QColor(0xff, 0xff, 0xff));

    paintOverImage(*painter, imageToDisplay, thumbToDisplay);
    return;
  }

  const QSizeF origImageSize(m_imageXform.origRect().size());
  const double xPreScale = origImageSize.width() / pixmap.width();
  const double yPreScale = origImageSize.height() / pixmap.height();
  QTransform preScaleXform;
  preScaleXform.scale(xPreScale, yPreScale);

  const QTransform pixmapToThumb(preScaleXform * m_imageXform.transform() * m_postScaleXform);

  // The polygon to draw into in original image coordinates.
  QPolygonF imagePoly
      = PolygonUtils::round(m_imageXform.resultingPreCropArea().intersected(m_imageXform.resultingRect()));
  // The polygon to draw into in display coordinates.
  QPolygonF displayPoly(imageToDisplay.map(imagePoly));

  QRectF displayRect(imageToDisplay.map(PolygonUtils::round(m_displayArea)).boundingRect().toAlignedRect());

  QPixmap tempPixmap;
  const QString cacheKey(QString::fromLatin1("ThumbnailBase::tempPixmap"));
  if (!QPixmapCache::find(cacheKey, &tempPixmap) || (tempPixmap.width() < displayRect.width())
      || (tempPixmap.height() < displayRect.height())) {
    auto w = (int) displayRect.width();
    auto h = (int) displayRect.height();
    // Add some extra, to avoid rectreating the pixmap too often.
    w += w / 10;
    h += h / 10;

    tempPixmap = QPixmap(w, h);

    if (!tempPixmap.hasAlphaChannel()) {
      // This actually forces the alpha channel to be created.
      tempPixmap.fill(Qt::transparent);
    }

    QPixmapCache::insert(cacheKey, tempPixmap);
  }

  QPainter tempPainter;
  tempPainter.begin(&tempPixmap);

  QTransform tempAdjustment;
  tempAdjustment.translate(-displayRect.left(), -displayRect.top());

  tempPainter.setWorldTransform(pixmapToThumb * thumbToDisplay * tempAdjustment);
  // Turn off alpha compositing.
  tempPainter.setCompositionMode(QPainter::CompositionMode_Source);
  tempPainter.setRenderHint(QPainter::SmoothPixmapTransform);
  tempPainter.setRenderHint(QPainter::Antialiasing);
  PixmapRenderer::drawPixmap(tempPainter, pixmap);

  // Turn alpha compositing on again.
  tempPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
  // Setup the painter for drawing in thumbnail coordinates,
  // as required for paintOverImage().
  tempPainter.setWorldTransform(thumbToDisplay * tempAdjustment);
  tempPainter.save();
  prePaintOverImage(tempPainter, imageToDisplay * tempAdjustment, thumbToDisplay * tempAdjustment);
  tempPainter.restore();

  tempPainter.setPen(Qt::NoPen);
  tempPainter.setBrush(Qt::transparent);
  tempPainter.setWorldTransform(tempAdjustment);
  tempPainter.setCompositionMode(QPainter::CompositionMode_Clear);
  {
    QPainterPath outerPath;
    outerPath.addRect(displayRect);
    QPainterPath innerPath;
    innerPath.addPolygon(displayPoly);
    tempPainter.drawPath(outerPath.subtracted(innerPath));
  }

  // Turn alpha compositing on again.
  tempPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
  // Setup the painter for drawing in thumbnail coordinates,
  // as required for paintOverImage().
  tempPainter.setWorldTransform(thumbToDisplay * tempAdjustment);
  tempPainter.save();
  paintOverImage(tempPainter, imageToDisplay * tempAdjustment, thumbToDisplay * tempAdjustment);
  tempPainter.restore();

  tempPainter.end();

  painter->setWorldTransform(QTransform());
  painter->setClipRect(displayRect);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
  painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
  painter->drawPixmap(displayRect.topLeft(), tempPixmap);
}  // ThumbnailBase::paint

void ThumbnailBase::paintDeviant(QPainter& painter) {
  if (!ApplicationSettings::getInstance().isHighlightDeviationEnabled()) {
    return;
  }

  QPen pen(QColor(0xdd, 0x00, 0x00, 0xcc));
  pen.setWidth(5);
  pen.setCosmetic(true);
  painter.setPen(pen);

  QFont font("Serif");
  font.setWeight(QFont::Bold);
  font.setPixelSize(static_cast<int>(boundingRect().width() / 2));
  painter.setFont(font);

  painter.drawText(boundingRect(), Qt::AlignCenter, "*");
}

void ThumbnailBase::setImageXform(const ImageTransformation& imageXform) {
  m_imageXform = imageXform;
  const QSizeF unscaledSize(m_displayArea.size().expandedTo(QSizeF(1, 1)));
  QSizeF scaledSize(unscaledSize);
  scaledSize.scale(m_maxSize, Qt::KeepAspectRatio);

  m_boundingRect = QRectF(QPointF(0.0, 0.0), scaledSize);

  const double xPostScale = scaledSize.width() / unscaledSize.width();
  const double yPostScale = scaledSize.height() / unscaledSize.height();
  m_postScaleXform.reset();
  m_postScaleXform.scale(xPostScale, yPostScale);
}

void ThumbnailBase::handleLoadResult(const ThumbnailLoadResult& result) {
  m_completionHandler.reset();

  if (result.status() != ThumbnailLoadResult::LOAD_FAILED) {
    // Note that we don't store result.pixmap() in
    // this object, because we may have already went
    // out of view, so we may never receive a paint event.
    update();
  }
}
