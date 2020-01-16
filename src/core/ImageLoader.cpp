// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageLoader.h"

#include <QFile>
#include <QImage>
#include <QtGui/QImageReader>

#include "ImageId.h"
#include "TiffReader.h"

QImage ImageLoader::load(const ImageId& imageId) {
  return load(imageId.filePath(), imageId.zeroBasedPage());
}

QImage ImageLoader::load(const QString& filePath, const int pageNum) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return QImage();
  }
  return load(file, pageNum);
}

QImage ImageLoader::load(QIODevice& ioDev, const int pageNum) {
  if (TiffReader::canRead(ioDev)) {
    return TiffReader::readImage(ioDev, pageNum);
  }

  if (pageNum != 0) {
    // Qt can only load the first page of multi-page images.
    return QImage();
  }

  QImage image;
  QImageReader(&ioDev).read(&image);
  return image;
}
