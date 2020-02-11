// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageMetadataLoader.h"

#include <QFile>
#include <QIODevice>
#include <QString>

#include "ImageMetadata.h"
#include "JpegMetadataLoader.h"
#include "PngMetadataLoader.h"
#include "TiffMetadataLoader.h"

ImageMetadataLoader::LoaderList ImageMetadataLoader::m_sLoaders;

void ImageMetadataLoader::registerLoader(std::shared_ptr<ImageMetadataLoader> loader) {
  m_sLoaders.push_back(std::move(loader));
}

ImageMetadataLoader::StaticInit::StaticInit() {
  registerLoader(std::make_shared<JpegMetadataLoader>());
  registerLoader(std::make_shared<PngMetadataLoader>());
  registerLoader(std::make_shared<TiffMetadataLoader>());
}

ImageMetadataLoader::StaticInit ImageMetadataLoader::m_staticInit;

ImageMetadataLoader::Status ImageMetadataLoader::loadImpl(QIODevice& ioDevice,
                                                          const VirtualFunction<void, const ImageMetadata&>& out) {
  auto it(m_sLoaders.begin());
  const auto end(m_sLoaders.end());
  for (; it != end; ++it) {
    const Status status = (*it)->loadMetadata(ioDevice, out);
    if (status != FORMAT_NOT_RECOGNIZED) {
      return status;
    }
  }
  return FORMAT_NOT_RECOGNIZED;
}

ImageMetadataLoader::Status ImageMetadataLoader::loadImpl(const QString& filePath,
                                                          const VirtualFunction<void, const ImageMetadata&>& out) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return GENERIC_ERROR;
  }
  return loadImpl(file, out);
}