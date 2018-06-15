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

#ifndef DEBUG_IMAGE_VIEW_H_
#define DEBUG_IMAGE_VIEW_H_

#include <QStackedWidget>
#include <QWidget>
#include <boost/function.hpp>
#include <boost/intrusive/list.hpp>
#include "AutoRemovingFile.h"

class QImage;

class DebugImageView
    : public QStackedWidget,
      public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
 public:
  explicit DebugImageView(AutoRemovingFile file,
                          const boost::function<QWidget*(const QImage&)>& image_view_factory
                          = boost::function<QWidget*(const QImage&)>(),
                          QWidget* parent = nullptr);

  /**
   * Tells this widget to either display the actual image or just
   * a placeholder.
   */
  void setLive(bool live);

 private:
  class ImageLoadResult;
  class ImageLoader;

  void imageLoaded(const QImage& image);

  AutoRemovingFile m_file;
  boost::function<QWidget*(const QImage&)> m_imageViewFactory;
  QWidget* m_pPlaceholderWidget;
  bool m_isLive;
};


#endif  // ifndef DEBUG_IMAGE_VIEW_H_
