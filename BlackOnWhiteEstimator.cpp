
#include "BlackOnWhiteEstimator.h"
#include <imageproc/Binarize.h>
#include <imageproc/Grayscale.h>
#include <imageproc/Morphology.h>
#include <imageproc/PolygonRasterizer.h>
#include <imageproc/RasterOp.h>
#include <imageproc/Transform.h>
#include "DebugImages.h"
#include "Despeckle.h"
#include "TaskStatus.h"

using namespace imageproc;

bool BlackOnWhiteEstimator::isBlackOnWhiteRefining(const imageproc::GrayImage& grayImage,
                                                   const ImageTransformation& xform,
                                                   const TaskStatus& status,
                                                   DebugImages* dbg) {
  BinaryImage bw150;
  {
    ImageTransformation xform150dpi(xform);
    xform150dpi.preScaleToDpi(Dpi(150, 150));

    if (xform150dpi.resultingRect().toRect().isEmpty()) {
      return true;
    }

    QImage gray150(transformToGray(grayImage, xform150dpi.transform(), xform150dpi.resultingRect().toRect(),
                                   OutsidePixels::assumeColor(Qt::white)));
    bw150 = binarizeOtsu(gray150);

    Despeckle::despeckleInPlace(bw150, Dpi(150, 150), Despeckle::NORMAL, status);
    bw150.invert();
    Despeckle::despeckleInPlace(bw150, Dpi(150, 150), Despeckle::NORMAL, status);
    bw150.invert();
    if (dbg) {
      dbg->add(bw150, "bw150");
    }
  }

  status.throwIfCancelled();

  BinaryImage contentMask;
  {
    BinaryImage whiteTopHat = whiteTopHatTransform(bw150, QSize(13, 13));
    BinaryImage blackTopHat = blackTopHatTransform(bw150, QSize(13, 13));

    contentMask = whiteTopHat;
    rasterOp<RopOr<RopSrc, RopDst>>(contentMask, blackTopHat);

    contentMask = closeBrick(contentMask, QSize(200, 200));
    contentMask = dilateBrick(contentMask, QSize(30, 30));
    if (dbg) {
      dbg->add(contentMask, "content_mask");
    }
  }

  status.throwIfCancelled();

  rasterOp<RopAnd<RopSrc, RopDst>>(bw150, contentMask);

  return (2 * bw150.countBlackPixels() <= contentMask.countBlackPixels());
}

bool BlackOnWhiteEstimator::isBlackOnWhite(const imageproc::GrayImage& grayImage,
                                           const ImageTransformation& xform,
                                           const TaskStatus& status,
                                           DebugImages* dbg) {
  if (isBlackOnWhite(grayImage, xform.resultingPreCropArea())) {
    return true;
  } else {
    // The black borders of the page can make the method above giving the wrong result.
    return isBlackOnWhiteRefining(grayImage, xform, status, dbg);
  }
}

bool BlackOnWhiteEstimator::isBlackOnWhite(const GrayImage& img, const BinaryImage& mask) {
  if (img.isNull()) {
    throw std::invalid_argument("BlackOnWhiteEstimator: image is null.");
  }
  if (img.size() != mask.size()) {
    throw std::invalid_argument("BlackOnWhiteEstimator: img and mask have different sizes");
  }

  BinaryImage bwImage(img, BinaryThreshold::otsuThreshold(GrayscaleHistogram(img, mask)));
  rasterOp<RopAnd<RopSrc, RopDst>>(bwImage, mask);

  return (2 * bwImage.countBlackPixels() <= mask.countBlackPixels());
}

bool BlackOnWhiteEstimator::isBlackOnWhite(const GrayImage& img, const QPolygonF& cropArea) {
  if (img.isNull()) {
    throw std::invalid_argument("BlackOnWhiteEstimator: image is null.");
  }
  if (cropArea.intersected(QRectF(img.rect())).isEmpty()) {
    throw std::invalid_argument("BlackOnWhiteEstimator: the cropping area is wrong.");
  }

  BinaryImage mask(img.size(), BLACK);
  PolygonRasterizer::fillExcept(mask, WHITE, cropArea, Qt::WindingFill);

  return isBlackOnWhite(img, mask);
}
