// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_BLACKONWHITEESTIMATOR_H_
#define SCANTAILOR_CORE_BLACKONWHITEESTIMATOR_H_

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


#endif  // SCANTAILOR_CORE_BLACKONWHITEESTIMATOR_H_
