
#include "ImageViewInfoObserver.h"
#include "ImageViewInfoProvider.h"

ImageViewInfoObserver::ImageViewInfoObserver() {
    ImageViewInfoProvider::getInstance()->attachObserver(this);
}

ImageViewInfoObserver::~ImageViewInfoObserver() {
    ImageViewInfoProvider::getInstance()->detachObserver(this);
}
