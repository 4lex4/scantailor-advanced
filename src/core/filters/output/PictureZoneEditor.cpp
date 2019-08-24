// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PictureZoneEditor.h"
#include <Constants.h>
#include <GrayImage.h>
#include <Transform.h>
#include <QPainter>
#include <QPointer>
#include <boost/bind.hpp>
#include <utility>
#include "BackgroundExecutor.h"
#include "ImagePresentation.h"
#include "ImageTransformation.h"
#include "OutputMargins.h"
#include "PictureLayerProperty.h"
#include "PictureZonePropDialog.h"
#include "PixmapRenderer.h"
#include "Settings.h"
#include "Zone.h"
#include "ZoneSet.h"

namespace output {
static const QRgb maskColor = 0xff587ff4;

using namespace imageproc;

class PictureZoneEditor::MaskTransformTask : public AbstractCommand<intrusive_ptr<AbstractCommand<void>>>,
                                             public QObject {
  DECLARE_NON_COPYABLE(MaskTransformTask)

 public:
  MaskTransformTask(PictureZoneEditor* zoneEditor,
                    const BinaryImage& mask,
                    const QTransform& xform,
                    const QSize& targetSize);

  void cancel() { m_result->cancel(); }

  bool isCancelled() const { return m_result->isCancelled(); }

  intrusive_ptr<AbstractCommand<void>> operator()() override;

 private:
  class Result : public AbstractCommand<void> {
   public:
    explicit Result(PictureZoneEditor* zoneEditor);

    void setData(const QPoint& origin, const QImage& mask);

    void cancel() { m_cancelFlag.fetchAndStoreRelaxed(1); }

    bool isCancelled() const { return m_cancelFlag.fetchAndAddRelaxed(0) != 0; }

    void operator()() override;

   private:
    QPointer<PictureZoneEditor> m_zoneEditor;
    QPoint m_origin;
    QImage m_mask;
    mutable QAtomicInt m_cancelFlag;
  };


  intrusive_ptr<Result> m_result;
  BinaryImage m_origMask;
  QTransform m_xform;
  QSize m_targetSize;
};


PictureZoneEditor::PictureZoneEditor(const QImage& image,
                                     const ImagePixmapUnion& downscaledImage,
                                     const imageproc::BinaryImage& pictureMask,
                                     const QTransform& imageToVirt,
                                     const QPolygonF& virtDisplayArea,
                                     const PageId& pageId,
                                     intrusive_ptr<Settings> settings)
    : ImageViewBase(image, downscaledImage, ImagePresentation(imageToVirt, virtDisplayArea), OutputMargins()),
      m_context(*this, m_zones),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_origPictureMask(pictureMask),
      m_pictureMaskAnimationPhase(270),
      m_pageId(pageId),
      m_settings(std::move(settings)) {
  m_zones.setDefaultProperties(m_settings->defaultPictureZoneProperties());

  setMouseTracking(true);

  m_context.setShowPropertiesCommand(boost::bind(&PictureZoneEditor::showPropertiesDialog, this, _1));

  connect(&m_zones, SIGNAL(committed()), SLOT(commitZones()));

  makeLastFollower(*m_context.createDefaultInteraction());

  rootInteractionHandler().makeLastFollower(*this);

  // We want these handlers after zone interaction handlers,
  // as some of those have their own drag and zoom handlers,
  // which need to get events before these standard ones.
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  connect(&m_pictureMaskAnimateTimer, SIGNAL(timeout()), SLOT(advancePictureMaskAnimation()));
  m_pictureMaskAnimateTimer.setSingleShot(true);
  m_pictureMaskAnimateTimer.setInterval(120);

  connect(&m_pictureMaskRebuildTimer, SIGNAL(timeout()), SLOT(initiateBuildingScreenPictureMask()));
  m_pictureMaskRebuildTimer.setSingleShot(true);
  m_pictureMaskRebuildTimer.setInterval(150);

  for (const Zone& zone : m_settings->pictureZonesForPage(pageId)) {
    auto spline = make_intrusive<EditableSpline>(zone.spline());
    m_zones.addZone(spline, zone.properties());
  }
}

PictureZoneEditor::~PictureZoneEditor() {
  m_settings->setDefaultPictureZoneProperties(m_zones.defaultProperties());
}

void PictureZoneEditor::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldTransform(QTransform());
  painter.setRenderHint(QPainter::Antialiasing);

  if (!validateScreenPictureMask()) {
    schedulePictureMaskRebuild();
  } else {
    const double sn = std::sin(constants::DEG2RAD * m_pictureMaskAnimationPhase);
    const double scale = 0.5 * (sn + 1.0);  // 0 .. 1
    const double opacity = 0.35 * scale + 0.15;

    QPixmap mask(m_screenPictureMask.rect().size());
    mask.fill(Qt::transparent);

    {
      QPainter maskPainter(&mask);
      maskPainter.drawPixmap(QPoint(0, 0), m_screenPictureMask);
      maskPainter.translate(-m_screenPictureMaskOrigin);
      paintOverPictureMask(maskPainter);
    }

    painter.setOpacity(opacity);
    painter.drawPixmap(m_screenPictureMaskOrigin, mask);
    painter.setOpacity(1.0);

    if (!m_pictureMaskAnimateTimer.isActive()) {
      m_pictureMaskAnimateTimer.start();
    }
  }
}

void PictureZoneEditor::advancePictureMaskAnimation() {
  m_pictureMaskAnimationPhase = (m_pictureMaskAnimationPhase + 40) % 360;
  update();
}

bool PictureZoneEditor::validateScreenPictureMask() const {
  return !m_screenPictureMask.isNull() && m_screenPictureMaskXform == virtualToWidget();
}

void PictureZoneEditor::schedulePictureMaskRebuild() {
  if (!m_pictureMaskRebuildTimer.isActive() || (m_potentialPictureMaskXform != virtualToWidget())) {
    if (m_maskTransformTask) {
      m_maskTransformTask->cancel();
      m_maskTransformTask.reset();
    }
    m_potentialPictureMaskXform = virtualToWidget();
  }
  m_pictureMaskRebuildTimer.start();
}

void PictureZoneEditor::initiateBuildingScreenPictureMask() {
  if (validateScreenPictureMask()) {
    return;
  }

  m_screenPictureMask = QPixmap();

  if (m_maskTransformTask) {
    m_maskTransformTask->cancel();
    m_maskTransformTask.reset();
  }

  const QTransform xform(virtualToWidget());
  const auto task = make_intrusive<MaskTransformTask>(this, m_origPictureMask, xform, viewport()->size());

  backgroundExecutor().enqueueTask(task);

  m_screenPictureMask = QPixmap();
  m_maskTransformTask = task;
  m_screenPictureMaskXform = xform;
}

void PictureZoneEditor::screenPictureMaskBuilt(const QPoint& origin, const QImage& mask) {
  m_screenPictureMask = QPixmap::fromImage(mask);
  m_screenPictureMaskOrigin = origin;
  m_pictureMaskAnimationPhase = 270;

  m_maskTransformTask.reset();
  update();
}

void PictureZoneEditor::paintOverPictureMask(QPainter& painter) {
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setTransform(imageToVirtual() * virtualToWidget(), true);
  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(maskColor));

  typedef PictureLayerProperty PLP;

  painter.setCompositionMode(QPainter::CompositionMode_Clear);

  // First pass: ERASER1
  for (const EditableZoneSet::Zone& zone : m_zones) {
    if (zone.properties()->locateOrDefault<PLP>()->layer() == PLP::ERASER1) {
      painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
    }
  }

  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  // Second pass: PAINTER2
  for (const EditableZoneSet::Zone& zone : m_zones) {
    if (zone.properties()->locateOrDefault<PLP>()->layer() == PLP::PAINTER2) {
      painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
    }
  }

  painter.setCompositionMode(QPainter::CompositionMode_Clear);

  // Third pass: ERASER3
  for (const EditableZoneSet::Zone& zone : m_zones) {
    if (zone.properties()->locateOrDefault<PLP>()->layer() == PLP::ERASER3) {
      painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
    }
  }
}  // PictureZoneEditor::paintOverPictureMask

void PictureZoneEditor::showPropertiesDialog(const EditableZoneSet::Zone& zone) {
  PropertySet savedProperties;
  zone.properties()->swap(savedProperties);
  *zone.properties() = savedProperties;

  PictureZonePropDialog dialog(zone.properties(), this);
  // We can't connect to the update() slot directly, as since some time,
  // Qt ignores such update requests on inactive windows.  Updating
  // it through a proxy slot does work though.
  connect(&dialog, SIGNAL(updated()), SLOT(updateRequested()));

  if (dialog.exec() == QDialog::Accepted) {
    m_zones.setDefaultProperties(*zone.properties());
    m_zones.commit();
    m_settings->setDefaultPictureZoneProperties(m_zones.defaultProperties());
  } else {
    zone.properties()->swap(savedProperties);
    update();
  }
}

void PictureZoneEditor::commitZones() {
  ZoneSet zones;

  for (const EditableZoneSet::Zone& zone : m_zones) {
    zones.add(Zone(*zone.spline(), *zone.properties()));
  }

  m_settings->setPictureZones(m_pageId, zones);

  emit invalidateThumbnail(m_pageId);
}

void PictureZoneEditor::updateRequested() {
  update();
}

/*============================= MaskTransformTask ===============================*/

PictureZoneEditor::MaskTransformTask::MaskTransformTask(PictureZoneEditor* zoneEditor,
                                                        const BinaryImage& mask,
                                                        const QTransform& xform,
                                                        const QSize& targetSize)
    : m_result(new Result(zoneEditor)), m_origMask(mask), m_xform(xform), m_targetSize(targetSize) {}

intrusive_ptr<AbstractCommand<void>> PictureZoneEditor::MaskTransformTask::operator()() {
  if (isCancelled()) {
    return nullptr;
  }

  const QRect targetRect(
      m_xform.map(QRectF(m_origMask.rect())).boundingRect().toRect().intersected(QRect(QPoint(0, 0), m_targetSize)));

  QImage grayMask(transformToGray(m_origMask.toQImage(), m_xform, targetRect, OutsidePixels::assumeWeakColor(Qt::black),
                                  QSizeF(0.0, 0.0)));

  QImage mask(grayMask.size(), QImage::Format_ARGB32_Premultiplied);
  mask.fill(maskColor);
  mask.setAlphaChannel(grayMask);

  m_result->setData(targetRect.topLeft(), mask);

  return m_result;
}

/*===================== MaskTransformTask::Result ===================*/

PictureZoneEditor::MaskTransformTask::Result::Result(PictureZoneEditor* zoneEditor) : m_zoneEditor(zoneEditor) {}

void PictureZoneEditor::MaskTransformTask::Result::setData(const QPoint& origin, const QImage& mask) {
  m_mask = mask;
  m_origin = origin;
}

void PictureZoneEditor::MaskTransformTask::Result::operator()() {
  if (m_zoneEditor && !isCancelled()) {
    m_zoneEditor->screenPictureMaskBuilt(m_origin, m_mask);
  }
}
}  // namespace output