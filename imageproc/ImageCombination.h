
#ifndef SCANTAILOR_IMAGECOMBINATIONOPERATIONS_H
#define SCANTAILOR_IMAGECOMBINATIONOPERATIONS_H

#include "BWColor.h"

class QImage;

namespace imageproc {
    class BinaryImage;

    void combineImageMono(QImage& mixedImage, const BinaryImage& foreground);

    void combineImageMono(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask);

    void combineImageColor(QImage& mixedImage, const QImage& foreground);

    void combineImageColor(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask);

    void combineImage(QImage& mixedImage, const QImage& foreground);

    void combineImage(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask);

    void applyMask(QImage& image, const BinaryImage& bw_mask, BWColor filling_color = WHITE);
}


#endif //SCANTAILOR_IMAGECOMBINATIONOPERATIONS_H
