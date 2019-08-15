/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
