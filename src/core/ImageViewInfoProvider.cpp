// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageViewInfoProvider.h"
#include <QtCore/QMutexLocker>
#include <QtCore/QRectF>
#include "Units.h"

ImageViewInfoProvider::ImageViewInfoProvider(const Dpi& dpi) : m_dpi(dpi) {}

ImageViewInfoProvider::~ImageViewInfoProvider() {
  for (ImageViewInfoListener* listener : m_infoListeners) {
    listener->onProviderStopped();
  }
}

void ImageViewInfoProvider::addListener(ImageViewInfoListener* listener) {
  listener->onDpiChanged(m_dpi);
  listener->onPhysSizeChanged(m_physSize);
  listener->onMousePosChanged(m_mousePos);

  m_infoListeners.push_back(listener);
}

void ImageViewInfoProvider::removeListener(ImageViewInfoListener* listener) {
  m_infoListeners.remove(listener);
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
  for (ImageViewInfoListener* listener : m_infoListeners) {
    listener->onPhysSizeChanged(physSize);
  }
}

void ImageViewInfoProvider::mousePosChanged(const QPointF& mousePos) const {
  for (ImageViewInfoListener* listener : m_infoListeners) {
    listener->onMousePosChanged(mousePos);
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
