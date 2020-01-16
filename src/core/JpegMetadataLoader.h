// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_JPEGMETADATALOADER_H_
#define SCANTAILOR_CORE_JPEGMETADATALOADER_H_

#include <vector>

#include "ImageMetadataLoader.h"
#include "VirtualFunction.h"

class QIODevice;
class ImageMetadata;

class JpegMetadataLoader : public ImageMetadataLoader {
 protected:
  Status loadMetadata(QIODevice& ioDevice, const VirtualFunction<void, const ImageMetadata&>& out) override;
};


#endif
