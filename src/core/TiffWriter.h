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
