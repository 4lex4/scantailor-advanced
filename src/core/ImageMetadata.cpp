// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageMetadata.h"
#include <Constants.h>

using namespace constants;

bool ImageMetadata::operator==(const ImageMetadata& other) const {
  if (m_size != other.m_size) {
    return false;
  } else if (m_dpi.isNull() && other.m_dpi.isNull()) {
    return true;
  } else {
    return m_dpi == other.m_dpi;
  }
}

bool ImageMetadata::isDpiOK() const {
  return horizontalDpiStatus() != DPI_UNDEFINED && verticalDpiStatus() != DPI_UNDEFINED;
}

ImageMetadata::DpiStatus ImageMetadata::horizontalDpiStatus() const {
  return dpiStatus(m_size.width(), m_dpi.horizontal());
}

ImageMetadata::DpiStatus ImageMetadata::verticalDpiStatus() const {
  return dpiStatus(m_size.height(), m_dpi.vertical());
}

ImageMetadata::DpiStatus ImageMetadata::dpiStatus(int pixelSize, int dpi) {
  if (dpi <= 1) {
    return DPI_UNDEFINED;
  }

  if (dpi < 150) {
    return DPI_TOO_SMALL;
  }

  if (dpi > 9999) {
    return DPI_TOO_LARGE;
  }

  const double mm = INCH2MM * pixelSize / dpi;
  if (mm > 500) {
    // This may indicate we are working with very large printed materials,
    // but most likely it indicates the DPI is wrong (too low).
    // DPIs that are too low may easily cause crashes due to out of memory
    // conditions.  The memory consumption is proportional to:
    // (real_hor_dpi / provided_hor_dpi) * (real_vert_dpi / provided_vert_dpi).
    // For example, if the real DPI is 600x600 but 200x200 is specified,
    // memory consumption is increased 9 times.
    return DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE;
  }

  return DPI_OK;
}
