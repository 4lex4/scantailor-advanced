
#include "StatusBarPanel.h"
#include <QtCore/QFileInfo>
#include <cmath>
#include "ImageViewInfoProvider.h"
#include "PageId.h"
#include "UnitsProvider.h"

StatusBarPanel::StatusBarPanel() {
  ui.setupUi(this);
}

void StatusBarPanel::updateMousePos(const QPointF& mousePos) {
  const QMutexLocker locker(&mutex);

  StatusBarPanel::mousePos = mousePos;
  mousePosChanged();
}

void StatusBarPanel::updatePhysSize(const QSizeF& physSize) {
  const QMutexLocker locker(&mutex);

  StatusBarPanel::physSize = physSize;
  physSizeChanged();
}

void StatusBarPanel::updateDpi(const Dpi& dpi) {
  StatusBarPanel::dpi = dpi;
}

void StatusBarPanel::clearImageViewInfo() {
  infoProvider = nullptr;
  updateMousePos(QPointF());
  updatePhysSize(QRectF().size());
  dpi = Dpi();
}

void StatusBarPanel::updatePage(int pageNumber, size_t pageCount, const PageId& pageId) {
  ui.pageNoLabel->setText(tr("p. %1 / %2").arg(pageNumber).arg(pageCount));

  QString pageFileInfo = QFileInfo(pageId.imageId().filePath()).baseName();
  if (pageFileInfo.size() > 15) {
    pageFileInfo = "..." + pageFileInfo.right(13);
  }
  if (pageId.subPage() != PageId::SINGLE_PAGE) {
    pageFileInfo = pageFileInfo.right(11) + ((pageId.subPage() == PageId::LEFT_PAGE) ? tr(" [L]") : tr(" [R]"));
  }

  ui.pageInfoLine->setVisible(true);
  ui.pageInfo->setText(pageFileInfo);
}

void StatusBarPanel::clear() {
  ui.mousePosLabel->clear();
  ui.physSizeLabel->clear();
  ui.pageNoLabel->clear();
  ui.pageInfo->clear();

  ui.mousePosLine->setVisible(false);
  ui.physSizeLine->setVisible(false);
  ui.pageInfoLine->setVisible(false);
}

void StatusBarPanel::updateUnits(Units) {
  const QMutexLocker locker(&mutex);

  mousePosChanged();
  physSizeChanged();
}

void StatusBarPanel::mousePosChanged() {
  if (!mousePos.isNull() && !dpi.isNull()) {
    double x = mousePos.x();
    double y = mousePos.y();
    UnitsProvider::getInstance()->convertFrom(x, y, PIXELS, dpi);

    switch (UnitsProvider::getInstance()->getUnits()) {
      case PIXELS:
      case MILLIMETRES:
        x = std::ceil(x);
        y = std::ceil(y);
        break;
      default:
        x = std::ceil(x * 10) / 10;
        y = std::ceil(y * 10) / 10;
        break;
    }

    ui.mousePosLine->setVisible(true);
    ui.mousePosLabel->setText(QString("%1, %2").arg(x).arg(y));
  } else {
    ui.mousePosLabel->clear();
    ui.mousePosLine->setVisible(false);
  }
}

void StatusBarPanel::physSizeChanged() {
  if (!physSize.isNull() && !dpi.isNull()) {
    double width = physSize.width();
    double height = physSize.height();
    UnitsProvider::getInstance()->convertFrom(width, height, PIXELS, dpi);

    const Units units = UnitsProvider::getInstance()->getUnits();
    switch (units) {
      case PIXELS:
        width = std::round(width);
        height = std::round(height);
        break;
      case MILLIMETRES:
        width = std::round(width);
        height = std::round(height);
        break;
      case CENTIMETRES:
        width = std::round(width * 10) / 10;
        height = std::round(height * 10) / 10;
        break;
      case INCHES:
        width = std::round(width * 10) / 10;
        height = std::round(height * 10) / 10;
        break;
    }

    ui.physSizeLine->setVisible(true);
    ui.physSizeLabel->setText(QString("%1 x %2 %3").arg(width).arg(height).arg(unitsToLocalizedString(units)));
  } else {
    ui.physSizeLabel->clear();
    ui.physSizeLine->setVisible(false);
  }
}

void StatusBarPanel::setInfoProvider(ImageViewInfoProvider* infoProvider) {
  if (this->infoProvider) {
    infoProvider->detachObserver(this);
  }
  if (infoProvider) {
    infoProvider->attachObserver(this);
  }

  this->infoProvider = infoProvider;
}
