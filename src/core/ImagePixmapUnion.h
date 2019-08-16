// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGE_PIXMAP_UNION_H_
#define IMAGE_PIXMAP_UNION_H_

#include <QImage>
#include <QPixmap>

class ImagePixmapUnion {
  // Member-wise copying is OK.
 public:
  ImagePixmapUnion() = default;

  ImagePixmapUnion(const QImage& image) : m_image(image) {}

  ImagePixmapUnion(const QPixmap& pixmap) : m_pixmap(pixmap) {}

  const QImage& image() const { return m_image; }

  const QPixmap& pixmap() const { return m_pixmap; }

  bool isNull() const { return m_image.isNull() && m_pixmap.isNull(); }

 private:
  QImage m_image;
  QPixmap m_pixmap;
};


#endif  // ifndef IMAGE_PIXMAP_UNION_H_
