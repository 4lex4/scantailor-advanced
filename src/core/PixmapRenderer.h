// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PIXMAPRENDERER_H_
#define PIXMAPRENDERER_H_

class QPainter;
class QPixmap;

class PixmapRenderer {
 public:
  /**
   * \brief Workarounds an issue with QPainter::drawPixmap().
   *
   * This method is more or less equivalent to:
   * \code
   * QPainter::drawPixmap(0, 0, pixmap);
   * \endcode
   * However, Qt's raster paint engine for some reason refuses to draw
   * the pixmap at all if a very strong zoom is applied (as of Qt 5.4.0).
   * This method solves the problem above by calculating the visible area
   * of the pixmap and communicating that information to QPainter.
   */
  static void drawPixmap(QPainter& painter, const QPixmap& pixmap);
};


#endif
