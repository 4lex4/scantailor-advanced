// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TIFFREADER_H_
#define TIFFREADER_H_

#include "ImageMetadataLoader.h"
#include "VirtualFunction.h"

class QIODevice;
class QImage;
class ImageMetadata;
class Dpi;

class TiffReader {
 public:
  static bool canRead(QIODevice& device);

  static ImageMetadataLoader::Status readMetadata(QIODevice& device,
                                                  const VirtualFunction<void, const ImageMetadata&>& out);

  /**
   * \brief Reads the image from io device to QImage.
   *
   * \param device The device to read from.  This device must be
   *        opened for reading and must be seekable.
   * \param page_num A zero-based page number within a multi-page
   *        TIFF file.
   * \return The resulting image, or a null image in case of failure.
   */
  static QImage readImage(QIODevice& device, int page_num = 0);

 private:
  class TiffHeader;
  class TiffHandle;

  struct TiffInfo;

  template <typename T>
  class TiffBuffer;

  static TiffHeader readHeader(QIODevice& device);

  static bool checkHeader(const TiffHeader& header);

  static ImageMetadata currentPageMetadata(const TiffHandle& tif);

  static Dpi getDpi(float xres, float yres, unsigned res_unit);

  static QImage extractBinaryOrIndexed8Image(const TiffHandle& tif, const TiffInfo& info);

  static void readLines(const TiffHandle& tif, QImage& image);

  static void readAndUnpackLines(const TiffHandle& tif, const TiffInfo& info, QImage& image);
};


#endif  // ifndef TIFFREADER_H_
