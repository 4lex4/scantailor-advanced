
#ifndef SCANTAILOR_SPLITIMAGE_H
#define SCANTAILOR_SPLITIMAGE_H


#include <QtGui/QImage>
#include <memory>
#include <functional>

namespace output {
    class SplitImage {
    public:
        SplitImage();

        SplitImage(const QImage& foreground, const QImage& background);

        QImage toImage() const;

        const QImage& getForegroundImage() const;

        void setForegroundImage(const QImage& foregroundImage);

        const QImage& getBackgroundImage() const;

        void setBackgroundImage(const QImage& backgroundImage);

        void applyToLayerImages(std::function<void(QImage&)> function);

        bool isNull() const;

    private:
        QImage foregroundImage;
        QImage backgroundImage;
    };
}


#endif //SCANTAILOR_SPLITIMAGE_H
