// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_FILTERDATA_H_
#define SCANTAILOR_CORE_FILTERDATA_H_

#include <BinaryThreshold.h>
#include <GrayImage.h>
#include <QImage>
#include "ImageSettings.h"
#include "ImageTransformation.h"

class FilterData {
  // Member-wise copying is OK.
 public:
  explicit FilterData(const QImage& image);

  FilterData(const FilterData& other, const ImageTransformation& xform);

  FilterData(const FilterData& other);

  imageproc::BinaryThreshold bwThreshold() const;

  const ImageTransformation& xform() const;

  const QImage& origImage() const;

  const imageproc::GrayImage& grayImage() const;

  bool isBlackOnWhite() const;

  imageproc::BinaryThreshold bwThresholdBlackOnWhite() const;

  imageproc::GrayImage grayImageBlackOnWhite() const;

  void updateImageParams(const ImageSettings::PageParams& imageParams);

 private:
  QImage m_origImage;
  imageproc::GrayImage m_grayImage;
  ImageTransformation m_xform;
  ImageSettings::PageParams m_imageParams;
};


#endif  // ifndef SCANTAILOR_CORE_FILTERDATA_H_
