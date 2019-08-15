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

#ifndef DEBUG_IMAGES_H_
#define DEBUG_IMAGES_H_

#include <QString>
#include <boost/function.hpp>
#include <deque>
#include "AutoRemovingFile.h"

class QImage;
class QWidget;

namespace imageproc {
class BinaryImage;
}

/**
 * \brief A sequence of image + label pairs.
 */
class DebugImages {
 public:
  virtual ~DebugImages() = default;

  virtual void add(const QImage& image,
                   const QString& label,
                   const boost::function<QWidget*(const QImage&)>& image_view_factory
                   = boost::function<QWidget*(const QImage&)>()) = 0;

  virtual void add(const imageproc::BinaryImage& image,
                   const QString& label,
                   const boost::function<QWidget*(const QImage&)>& image_view_factory
                   = boost::function<QWidget*(const QImage&)>()) = 0;

  virtual bool empty() const = 0;

  /**
   * \brief Removes and returns the first item in the sequence.
   *
   * The label and viewer widget factory (that may not be bound)
   * are returned by taking pointers to them as arguments.
   * Returns a null AutoRemovingFile if image sequence is empty.
   */
  virtual AutoRemovingFile retrieveNext(QString* label = nullptr,
                                        boost::function<QWidget*(const QImage&)>* image_view_factory = nullptr) = 0;
};


#endif  // ifndef DEBUG_IMAGES_H_
