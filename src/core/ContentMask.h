// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_CONTENTMASK_H_
#define SCANTAILOR_CORE_CONTENTMASK_H_

#include <imageproc/BinaryImage.h>

#include <QtGui/QTransform>

namespace imageproc {
class GrayImage;
}
class ImageTransformation;
class TaskStatus;

class ContentMask {
 public:
  ContentMask() = default;

  ContentMask(const imageproc::GrayImage& grayImage, const ImageTransformation& xform, const TaskStatus& status);

  QRect findContentInArea(const QRect& area) const;

  const imageproc::BinaryImage& image() const { return m_image; }

  const QTransform& originalToContentXform() const { return m_originalToContentXform; }

  const QTransform& contentToOriginalXform() const { return m_contentToOriginalXform; }

  bool isNull() const { return m_image.isNull(); }

 private:
  imageproc::BinaryImage m_image;
  QTransform m_originalToContentXform;
  QTransform m_contentToOriginalXform;
};


#endif  // SCANTAILOR_CORE_CONTENTMASK_H_
