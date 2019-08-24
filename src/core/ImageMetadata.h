// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEMETADATA_H_
#define SCANTAILOR_CORE_IMAGEMETADATA_H_

#include <QSize>
#include "Dpi.h"

class ImageMetadata {
  // Member-wise copying is OK.
 public:
  enum DpiStatus { DPI_OK, DPI_UNDEFINED, DPI_TOO_LARGE, DPI_TOO_SMALL, DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE };

  ImageMetadata() = default;

  ImageMetadata(QSize size, Dpi dpi) : m_size(size), m_dpi(dpi) {}

  const QSize& size() const { return m_size; }

  void setSize(const QSize& size) { m_size = size; }

  const Dpi& dpi() const { return m_dpi; }

  void setDpi(const Dpi& dpi) { m_dpi = dpi; }

  bool isDpiOK() const;

  DpiStatus horizontalDpiStatus() const;

  DpiStatus verticalDpiStatus() const;

  bool operator==(const ImageMetadata& other) const;

  bool operator!=(const ImageMetadata& other) const { return !(*this == other); }

 private:
  static DpiStatus dpiStatus(int pixelSize, int dpi);

  QSize m_size;
  Dpi m_dpi;
};


#endif  // ifndef SCANTAILOR_CORE_IMAGEMETADATA_H_
