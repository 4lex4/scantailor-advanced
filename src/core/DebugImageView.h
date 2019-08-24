// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_DEBUGIMAGEVIEW_H_
#define SCANTAILOR_CORE_DEBUGIMAGEVIEW_H_

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
  QWidget* m_placeholderWidget;
  bool m_isLive;
};


#endif  // ifndef SCANTAILOR_CORE_DEBUGIMAGEVIEW_H_
