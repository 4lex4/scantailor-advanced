/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ImageMetadataLoader.h"
#include <QFile>
#include <QIODevice>
#include <QString>
#include "ImageMetadata.h"

ImageMetadataLoader::LoaderList ImageMetadataLoader::m_sLoaders;

void ImageMetadataLoader::registerLoader(intrusive_ptr<ImageMetadataLoader> loader) {
  m_sLoaders.push_back(std::move(loader));
}

ImageMetadataLoader::Status ImageMetadataLoader::loadImpl(QIODevice& io_device,
                                                          const VirtualFunction<void, const ImageMetadata&>& out) {
  auto it(m_sLoaders.begin());
  const auto end(m_sLoaders.end());
  for (; it != end; ++it) {
    const Status status = (*it)->loadMetadata(io_device, out);
    if (status != FORMAT_NOT_RECOGNIZED) {
      return status;
    }
  }

  return FORMAT_NOT_RECOGNIZED;
}

ImageMetadataLoader::Status ImageMetadataLoader::loadImpl(const QString& file_path,
                                                          const VirtualFunction<void, const ImageMetadata&>& out) {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    return GENERIC_ERROR;
  }

  return loadImpl(file, out);
}
