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

#include "PictureZoneEditor.h"
#include <QPainter>
#include <QPointer>
#include <boost/bind.hpp>
#include <utility>
#include "BackgroundExecutor.h"
#include "ImagePresentation.h"
#include "ImageTransformation.h"
#include "OutputMargins.h"
#include "PictureZonePropDialog.h"
#include "PixmapRenderer.h"
#include "Settings.h"
#include "Zone.h"
#include "ZoneSet.h"
#include "imageproc/Constants.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Transform.h"

namespace output {
static const QRgb mask_color = 0xff587ff4;

using namespace imageproc;

class PictureZoneEditor::MaskTransformTask : public AbstractCommand<intrusive_ptr<AbstractCommand<void>>>,
                                             public QObject {
  DECLARE_NON_COPYABLE(MaskTransformTask)

 public:
  MaskTransformTask(PictureZoneEditor* zone_editor,
                    const BinaryImage& mask,
                    const QTransform& xform,
                    const QSize& target_size);

  void cancel() { m_ptrResult->cancel(); }

  const bool isCancelled() const { return m_ptrResult->isCancelled(); }

  intrusive_ptr<AbstractCommand<void>> operator()() override;

 private:
  class Result : public AbstractCommand<void> {
   public:
    explicit Result(PictureZoneEditor* zone_editor);

    void setData(const QPoint& origin, const QImage& mask);

    void cancel() { m_cancelFlag.fetchAndStoreRelaxed(1); }

    bool isCancelled() const { return m_cancelFlag.fetchAndAddRelaxed(0) != 0; }

    void operator()() override;

   private:
    QPointer<PictureZoneEditor> m_ptrZoneEditor;
    QPoint m_origin;
    QImage m_mask;
    mutable QAtomicInt m_cancelFlag;
  };


  intrusive_ptr<Result> m_ptrResult;
  BinaryImage m_origMask;
  QTransform m_xform;
  QSize m_targetSize;
};


PictureZoneEditor::PictureZoneEditor(const QImage& image,
                                     const ImagePixmapUnion& downscaled_image,
                                     const imageproc::BinaryImage& picture_mask,
                                     const QTransform& image_to_virt,
                                     const QPolygonF& virt_display_area,
                                     const PageId& page_id,
                                     intrusive_ptr<Settings> settings)
    : ImageViewBase(image, downscaled_image, ImagePresentation(image_to_virt, virt_display_area), OutputMargins()),
      m_context(*this, m_zones),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_origPictureMask(picture_mask),
      m_pictureMaskAnimationPhase(270),
      m_pageId(page_id),
      m_ptrSettings(std::move(settings)) {
  m_zones.setDefaultProperties(m_ptrSettings->defaultPictureZoneProperties());

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

  for (const Zone& zone : m_ptrSettings->pictureZonesForPage(page_id)) {
    auto spline = make_intrusive<EditableSpline>(zone.spline());
    m_zones.addZone(spline, zone.properties());
  }
}

PictureZoneEditor::~PictureZoneEditor() {
  m_ptrSettings->setDefaultPictureZoneProperties(m_zones.defaultProperties());
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
      QPainter mask_painter(&mask);
      mask_painter.drawPixmap(QPoint(0, 0), m_screenPictureMask);
      mask_painter.translate(-m_screenPictureMaskOrigin);
      paintOverPictureMask(mask_painter);
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
    if (m_ptrMaskTransformTask) {
      m_ptrMaskTransformTask->cancel();
      m_ptrMaskTransformTask.reset();
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

  if (m_ptrMaskTransformTask) {
    m_ptrMaskTransformTask->cancel();
    m_ptrMaskTransformTask.reset();
  }

  const QTransform xform(virtualToWidget());
  const auto task = make_intrusive<MaskTransformTask>(this, m_origPictureMask, xform, viewport()->size());

  backgroundExecutor().enqueueTask(task);

  m_screenPictureMask = QPixmap();
  m_ptrMaskTransformTask = task;
  m_screenPictureMaskXform = xform;
}

void PictureZoneEditor::screenPictureMaskBuilt(const QPoint& origin, const QImage& mask) {
  m_screenPictureMask = QPixmap::fromImage(mask);
  m_screenPictureMaskOrigin = origin;
  m_pictureMaskAnimationPhase = 270;

  m_ptrMaskTransformTask.reset();
  update();
}

void PictureZoneEditor::paintOverPictureMask(QPainter& painter) {
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setTransform(imageToVirtual() * virtualToWidget(), true);
  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(mask_color));

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
  PropertySet saved_properties;
  zone.properties()->swap(saved_properties);
  *zone.properties() = saved_properties;

  PictureZonePropDialog dialog(zone.properties(), this);
  // We can't connect to the update() slot directly, as since some time,
  // Qt ignores such update requests on inactive windows.  Updating
  // it through a proxy slot does work though.
  connect(&dialog, SIGNAL(updated()), SLOT(updateRequested()));

  if (dialog.exec() == QDialog::Accepted) {
    m_zones.setDefaultProperties(*zone.properties());
    m_zones.commit();
    m_ptrSettings->setDefaultPictureZoneProperties(m_zones.defaultProperties());
  } else {
    zone.properties()->swap(saved_properties);
    update();
  }
}

void PictureZoneEditor::commitZones() {
  ZoneSet zones;

  for (const EditableZoneSet::Zone& zone : m_zones) {
    zones.add(Zone(*zone.spline(), *zone.properties()));
  }

  m_ptrSettings->setPictureZones(m_pageId, zones);

  emit invalidateThumbnail(m_pageId);
}

void PictureZoneEditor::updateRequested() {
  update();
}

/*============================= MaskTransformTask ===============================*/

PictureZoneEditor::MaskTransformTask::MaskTransformTask(PictureZoneEditor* zone_editor,
                                                        const BinaryImage& mask,
                                                        const QTransform& xform,
                                                        const QSize& target_size)
    : m_ptrResult(new Result(zone_editor)), m_origMask(mask), m_xform(xform), m_targetSize(target_size) {}

intrusive_ptr<AbstractCommand<void>> PictureZoneEditor::MaskTransformTask::operator()() {
  if (isCancelled()) {
    return nullptr;
  }

  const QRect target_rect(
      m_xform.map(QRectF(m_origMask.rect())).boundingRect().toRect().intersected(QRect(QPoint(0, 0), m_targetSize)));

  QImage gray_mask(transformToGray(m_origMask.toQImage(), m_xform, target_rect,
                                   OutsidePixels::assumeWeakColor(Qt::black), QSizeF(0.0, 0.0)));

  QImage mask(gray_mask.size(), QImage::Format_ARGB32_Premultiplied);
  mask.fill(mask_color);
  mask.setAlphaChannel(gray_mask);

  m_ptrResult->setData(target_rect.topLeft(), mask);

  return m_ptrResult;
}

/*===================== MaskTransformTask::Result ===================*/

PictureZoneEditor::MaskTransformTask::Result::Result(PictureZoneEditor* zone_editor) : m_ptrZoneEditor(zone_editor) {}

void PictureZoneEditor::MaskTransformTask::Result::setData(const QPoint& origin, const QImage& mask) {
  m_mask = mask;
  m_origin = origin;
}

void PictureZoneEditor::MaskTransformTask::Result::operator()() {
  if (m_ptrZoneEditor && !isCancelled()) {
    m_ptrZoneEditor->screenPictureMaskBuilt(m_origin, m_mask);
  }
}
}  // namespace output