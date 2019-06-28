#ifndef SCANTAILOR_OUTPUTIMAGEBUILDER_H
#define SCANTAILOR_OUTPUTIMAGEBUILDER_H

#include <memory>
#include "OutputImage.h"

enum class ForegroundType;
namespace imageproc {
class BinaryImage;
}

namespace output {
class OutputImageBuilder {
 public:
  ~OutputImageBuilder();

  std::unique_ptr<OutputImage> build();

  OutputImageBuilder& setImage(const QImage& image);

  OutputImageBuilder& setForegroundImage(const QImage& foregroundImage);

  OutputImageBuilder& setBackgroundImage(const QImage& backgroundImage);

  OutputImageBuilder& setOriginalBackgroundImage(const QImage& originalBackgroundImage);

  OutputImageBuilder& setForegroundType(const ForegroundType& foregroundImage);

  OutputImageBuilder& setForegroundMask(const imageproc::BinaryImage& foregroundMask);

  OutputImageBuilder& setBackgroundMask(const imageproc::BinaryImage& backgroundMask);

 private:
  std::unique_ptr<QImage> m_image;
  std::unique_ptr<QImage> m_foregroundImage;
  std::unique_ptr<QImage> m_backgroundImage;
  std::unique_ptr<QImage> m_originalBackgroundImage;
  std::unique_ptr<ForegroundType> m_foregroundType;
  std::unique_ptr<imageproc::BinaryImage> m_foregroundMask;
  std::unique_ptr<imageproc::BinaryImage> m_backgroundMask;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUTIMAGEBUILDER_H
