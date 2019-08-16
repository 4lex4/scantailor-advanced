// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TIFFWRITER_H_
#define TIFFWRITER_H_

#include <tiff.h>
#include <cstddef>
#include <cstdint>

class QIODevice;
class QString;
class QImage;
class Dpm;

class TiffWriter {
 public:
  /**
   * \brief Writes a QImage in TIFF format to a file.
   *
   * \param file_path The full path to the file.
   * \param image The image to write.  Writing a null image will fail.
   * \return True on success, false on failure.
   */
  static bool writeImage(const QString& file_path, const QImage& image);

  /**
   * \brief Writes a QImage in TIFF format to an IO device.
   *
   * \param device The device to write to.  This device must be
   *        opened for writing and seekable.
   * \param image The image to write.  Writing a null image will fail.
   * \return True on success, false on failure.
   */
  static bool writeImage(QIODevice& device, const QImage& image);

 private:
  class TiffHandle;

  static void setDpm(const TiffHandle& tif, const Dpm& dpm);

  static bool writeBitonalOrIndexed8Image(const TiffHandle& tif, const QImage& image);

  static bool writeRGB32Image(const TiffHandle& tif, const QImage& image);

  static bool writeARGB32Image(const TiffHandle& tif, const QImage& image);

  static bool write8bitLines(const TiffHandle& tif, const QImage& image);

  static bool writeBinaryLinesAsIs(const TiffHandle& tif, const QImage& image);

  static bool writeBinaryLinesReversed(const TiffHandle& tif, const QImage& image);

  static const uint8_t m_reverseBitsLUT[256];
};


#endif  // ifndef TIFFWRITER_H_
