
#ifndef SCANTAILOR_BLACKONWHITEESTIMATOR_H
#define SCANTAILOR_BLACKONWHITEESTIMATOR_H

#include <imageproc/BinaryImage.h>
#include <QtGui/QPolygonF>
#include "ImageTransformation.h"

class TaskStatus;
class DebugImages;

namespace imageproc {
class GrayImage;
class BinaryImage;
}  // namespace imageproc

class BlackOnWhiteEstimator {
 public:
  static bool isBlackOnWhite(const imageproc::GrayImage& grayImage,
                             const ImageTransformation& xform,
                             const TaskStatus& status,
                             DebugImages* dbg = nullptr);

  static bool isBlackOnWhiteRefining(const imageproc::GrayImage& grayImage,
                                     const ImageTransformation& xform,
                                     const TaskStatus& status,
                                     DebugImages* dbg = nullptr);

  static bool isBlackOnWhite(const imageproc::GrayImage& img, const imageproc::BinaryImage& mask);

  static bool isBlackOnWhite(const imageproc::GrayImage& img, const QPolygonF& cropArea);
};


#endif  // SCANTAILOR_BLACKONWHITEESTIMATOR_H
