/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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


#ifndef OUTPUT_DESPECKLE_VISUALIZATION_H_
#define OUTPUT_DESPECKLE_VISUALIZATION_H_

#include <QImage>

class Dpi;

namespace imageproc {
class BinaryImage;
}

namespace output {
class DespeckleVisualization {
 public:
  /*
   * Constructs a null visualization.
   */
  DespeckleVisualization() = default;

  /**
   * \param output The output file, as produced by OutputGenerator::process().
   *        If this one is null, the visualization will be null as well.
   * \param speckles Speckles detected in the image.
   *        If this one is null, it is considered no speckles were detected.
   * \param dpi Dots-per-inch of both images.
   */
  DespeckleVisualization(const QImage& output, const imageproc::BinaryImage& speckles, const Dpi& dpi);

  bool isNull() const;

  const QImage& image() const;

  const QImage& downscaledImage() const;

 private:
  static void colorizeSpeckles(QImage& image, const imageproc::BinaryImage& speckles, const Dpi& dpi);


  QImage m_image;
  QImage m_downscaledImage;
};
}  // namespace output
#endif  // ifndef OUTPUT_DESPECKLE_VISUALIZATION_H_
