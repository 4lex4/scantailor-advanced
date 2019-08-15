
#ifndef SCANTAILOR_IMAGECOMBINATIONOPERATIONS_H
#define SCANTAILOR_IMAGECOMBINATIONOPERATIONS_H

#include "BWColor.h"

class QImage;

namespace imageproc {
class BinaryImage;

void combineImages(QImage& mixedImage, const BinaryImage& foreground);

void combineImages(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask);

void combineImages(QImage& mixedImage, const QImage& foreground);

void combineImages(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask);

void applyMask(QImage& image, const BinaryImage& bw_mask, BWColor filling_color = WHITE);
}  // namespace imageproc


#endif  // SCANTAILOR_IMAGECOMBINATIONOPERATIONS_H
