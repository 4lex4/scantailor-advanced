
#include "ImageViewInfoProvider.h"
#include <QtCore/QMutexLocker>
#include <QtCore/QRectF>
#include "Units.h"

ImageViewInfoProvider::ImageViewInfoProvider(const Dpi& dpi) : dpi(dpi){};

ImageViewInfoProvider::~ImageViewInfoProvider() {
  for (ImageViewInfoObserver* observer : observers) {
    observer->clearImageViewInfo();
  }
}

void ImageViewInfoProvider::attachObserver(ImageViewInfoObserver* observer) {
  observer->updateDpi(dpi);
  observer->updatePhysSize(physSize);
  observer->updateMousePos(mousePos);

  observers.push_back(observer);
}

void ImageViewInfoProvider::detachObserver(ImageViewInfoObserver* observer) {
  observer->clearImageViewInfo();

  observers.remove(observer);
}

void ImageViewInfoProvider::setPhysSize(const QSizeF& physSize) {
  ImageViewInfoProvider::physSize = physSize;
  physSizeChanged(physSize);
}

void ImageViewInfoProvider::setMousePos(const QPointF& mousePos) {
  ImageViewInfoProvider::mousePos = mousePos;
  mousePosChanged(mousePos);
}

void ImageViewInfoProvider::physSizeChanged(const QSizeF& physSize) const {
  for (ImageViewInfoObserver* observer : observers) {
    observer->updatePhysSize(physSize);
  }
}

void ImageViewInfoProvider::mousePosChanged(const QPointF& mousePos) const {
  for (ImageViewInfoObserver* observer : observers) {
    observer->updateMousePos(mousePos);
  }
}

const Dpi& ImageViewInfoProvider::getDpi() const {
  return dpi;
}

const QPointF& ImageViewInfoProvider::getMousePos() const {
  return mousePos;
}

const QSizeF& ImageViewInfoProvider::getPhysSize() const {
  return physSize;
}
