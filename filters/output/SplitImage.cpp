
#include "SplitImage.h"

namespace output {
    QImage SplitImage::toImage() const {
        return QImage();
    }

    const QImage& SplitImage::getForegroundImage() const {
        return foregroundImage;
    }

    void SplitImage::setForegroundImage(const QImage& foregroundImage) {
        SplitImage::foregroundImage = foregroundImage;
    }

    const QImage& SplitImage::getBackgroundImage() const {
        return backgroundImage;
    }

    void SplitImage::setBackgroundImage(const QImage& backgroundImage) {
        SplitImage::backgroundImage = backgroundImage;
    }

    void SplitImage::performWithImages(std::function<void(QImage&)> function) {
        function(foregroundImage);
        function(backgroundImage);
    }
}
