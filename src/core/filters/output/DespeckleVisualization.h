// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


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
