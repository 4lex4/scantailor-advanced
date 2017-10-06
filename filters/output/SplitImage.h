
#ifndef SCANTAILOR_SPLITIMAGE_H
#define SCANTAILOR_SPLITIMAGE_H


#include <QtGui/QImage>
#include <memory>
#include <functional>

namespace output {
    class SplitImage {
    public:
        QImage toImage() const;

        const QImage& getForegroundImage() const;

        void setForegroundImage(const QImage& foregroundImage);

        const QImage& getBackgroundImage() const;

        void setBackgroundImage(const QImage& backgroundImage);

        void performWithImages(std::function<void(QImage&)> function);

    private:
        QImage foregroundImage;
        QImage backgroundImage;
    };
}


#endif //SCANTAILOR_SPLITIMAGE_H
