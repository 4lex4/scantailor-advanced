// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TiffMetadataLoader.h"
#include "TiffReader.h"

ImageMetadataLoader::Status TiffMetadataLoader::loadMetadata(QIODevice& io_device,
                                                             const VirtualFunction<void, const ImageMetadata&>& out) {
  return TiffReader::readMetadata(io_device, out);
}
