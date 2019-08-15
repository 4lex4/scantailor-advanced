
#include "ImageViewInfoProvider.h"
#include <QtCore/QMutexLocker>
#include <QtCore/QRectF>
#include "Units.h"

ImageViewInfoProvider::ImageViewInfoProvider(const Dpi& dpi) : m_dpi(dpi){};

ImageViewInfoProvider::~ImageViewInfoProvider() {
  for (ImageViewInfoObserver* observer : m_observers) {
    observer->clearImageViewInfo();
  }
}

void ImageViewInfoProvider::attachObserver(ImageViewInfoObserver* observer) {
  observer->updateDpi(m_dpi);
  observer->updatePhysSize(m_physSize);
  observer->updateMousePos(m_mousePos);

  m_observers.push_back(observer);
}

void ImageViewInfoProvider::detachObserver(ImageViewInfoObserver* observer) {
  observer->clearImageViewInfo();

  m_observers.remove(observer);
}

void ImageViewInfoProvider::setPhysSize(const QSizeF& physSize) {
  ImageViewInfoProvider::m_physSize = physSize;
  physSizeChanged(physSize);
}

void ImageViewInfoProvider::setMousePos(const QPointF& mousePos) {
  ImageViewInfoProvider::m_mousePos = mousePos;
  mousePosChanged(mousePos);
}

void ImageViewInfoProvider::physSizeChanged(const QSizeF& physSize) const {
  for (ImageViewInfoObserver* observer : m_observers) {
    observer->updatePhysSize(physSize);
  }
}

void ImageViewInfoProvider::mousePosChanged(const QPointF& mousePos) const {
  for (ImageViewInfoObserver* observer : m_observers) {
    observer->updateMousePos(mousePos);
  }
}

const Dpi& ImageViewInfoProvider::getDpi() const {
  return m_dpi;
}

const QPointF& ImageViewInfoProvider::getMousePos() const {
  return m_mousePos;
}

const QSizeF& ImageViewInfoProvider::getPhysSize() const {
  return m_physSize;
}
