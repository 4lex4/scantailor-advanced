// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageLoader.h"
#include <QFile>
#include <QImage>
#include <QtGui/QImageReader>
#include "ImageId.h"
#include "TiffReader.h"

QImage ImageLoader::load(const ImageId& image_id) {
  return load(image_id.filePath(), image_id.zeroBasedPage());
}

QImage ImageLoader::load(const QString& file_path, const int page_num) {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QImage();
  }

  return load(file, page_num);
}

QImage ImageLoader::load(QIODevice& io_dev, const int page_num) {
  if (TiffReader::canRead(io_dev)) {
    return TiffReader::readImage(io_dev, page_num);
  }

  if (page_num != 0) {
    // Qt can only load the first page of multi-page images.
    return QImage();
  }

  QImage image;
  QImageReader(&io_dev).read(&image);

  return image;
}
