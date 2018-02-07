
#include <QtCore/QRectF>
#include "ImageViewInfoProvider.h"

std::unique_ptr<ImageViewInfoProvider> ImageViewInfoProvider::instance = nullptr;

ImageViewInfoProvider::ImageViewInfoProvider()
        : physSize(QRectF().size()) {
}

ImageViewInfoProvider* ImageViewInfoProvider::getInstance() {
    if (instance == nullptr) {
        instance.reset(new ImageViewInfoProvider());
    }

    return instance.get();
}

void ImageViewInfoProvider::attachObserver(ImageViewInfoObserver* observer) {
    observers.push_back(observer);
}

void ImageViewInfoProvider::detachObserver(ImageViewInfoObserver* observer) {
    auto it = std::find(observers.begin(), observers.end(), observer);
    if (it != observers.end()) {
        observers.erase(it);
    }
}

const QSizeF& ImageViewInfoProvider::getPhysSize() const {
    return physSize;
}

void ImageViewInfoProvider::setPhysSize(const QSizeF& physSize) {
    ImageViewInfoProvider::physSize = physSize;
    physSizeChanged();
}

const QPointF& ImageViewInfoProvider::getMousePos() const {
    return mousePos;
}

void ImageViewInfoProvider::setMousePos(const QPointF& mousePos) {
    ImageViewInfoProvider::mousePos = mousePos;
    mousePosChanged();
}

void ImageViewInfoProvider::physSizeChanged() {
    for (ImageViewInfoObserver* observer : observers) {
        observer->updatePhysSize(physSize);
    }
}

void ImageViewInfoProvider::mousePosChanged() {
    for (ImageViewInfoObserver* observer : observers) {
        observer->updateMousePos(mousePos);
    }
}
