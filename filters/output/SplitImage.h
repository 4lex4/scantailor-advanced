
#ifndef SCANTAILOR_SPLITIMAGE_H
#define SCANTAILOR_SPLITIMAGE_H


#include <QtGui/QImage>
#include <memory>
#include <functional>
#include <imageproc/BinaryImage.h>

namespace output {

    /**
     * The class to store and manage output images split to foreground and background layers.
     * It works in two modes:
     *  1.  Storing foreground and background images directly.
     *      Then use \code SplitImage::toImage() method to get the image from layers.
     *  2.  Storing only an image and mask.
     *      Then use \code SplitImage::getForegroundImage() and
     *      \code SplitImage::getBackgroundImage() to get the image layers.
     */
    class SplitImage {
    public:
        SplitImage();

        SplitImage(const QImage& foreground, const QImage& background);

        QImage toImage() const;

        QImage getForegroundImage() const;

        void setForegroundImage(const QImage& foregroundImage);

        QImage getBackgroundImage() const;

        void setBackgroundImage(const QImage& backgroundImage);

        void setMask(const imageproc::BinaryImage& mask, bool binariryForeground);

        void applyToLayerImages(std::function<void(QImage&)> function);

        bool isNull() const;

    private:
        bool binaryForeground;
        imageproc::BinaryImage mask;
        QImage foregroundImage;
        QImage backgroundImage;
    };
}


#endif //SCANTAILOR_SPLITIMAGE_H
