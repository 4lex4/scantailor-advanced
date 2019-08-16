// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_POSTERIZER_H
#define SCANTAILOR_POSTERIZER_H


#include <QtGui/QImage>
#include <unordered_map>

namespace imageproc {
class Posterizer {
 public:
  explicit Posterizer(int level,
                      bool normalize = false,
                      bool forceBlackAndWhite = false,
                      int normalizeBlackLevel = 0,
                      int normalizeWhiteLevel = 255);

  static QVector<QRgb> buildPalette(const QImage& image);

  static QImage convertToIndexed(const QImage& image);

  static QImage convertToIndexed(const QImage& image, const QVector<QRgb>& palette);

  QImage posterize(const QImage& image) const;

 private:
  int m_level;
  bool m_normalize;
  bool m_forceBlackAndWhite;
  int m_normalizeBlackLevel;
  int m_normalizeWhiteLevel;
};
}  // namespace imageproc

#endif  // SCANTAILOR_POSTERIZER_H
