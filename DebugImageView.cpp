/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "DebugImageView.h"
#include <QPointer>
#include <utility>
#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "BasicImageView.h"
#include "ImageViewBase.h"
#include "ProcessingIndicationWidget.h"

class DebugImageView::ImageLoadResult : public AbstractCommand<void> {
 public:
  ImageLoadResult(QPointer<DebugImageView> owner, const QImage& image) : m_ptrOwner(std::move(owner)), m_image(image) {}

  // This method is called from the main thread.
  void operator()() override {
    if (DebugImageView* owner = m_ptrOwner) {
      owner->imageLoaded(m_image);
    }
  }

 private:
  QPointer<DebugImageView> m_ptrOwner;
  QImage m_image;
};


class DebugImageView::ImageLoader : public AbstractCommand<BackgroundExecutor::TaskResultPtr> {
 public:
  ImageLoader(DebugImageView* owner, const QString& file_path) : m_ptrOwner(owner), m_filePath(file_path) {}

  BackgroundExecutor::TaskResultPtr operator()() override {
    QImage image(m_filePath);

    return make_intrusive<ImageLoadResult>(m_ptrOwner, image);
  }

 private:
  QPointer<DebugImageView> m_ptrOwner;
  QString m_filePath;
};


DebugImageView::DebugImageView(AutoRemovingFile file,
                               const boost::function<QWidget*(const QImage&)>& image_view_factory,
                               QWidget* parent)
    : QStackedWidget(parent),
      m_file(file),
      m_imageViewFactory(image_view_factory),
      m_pPlaceholderWidget(new ProcessingIndicationWidget(this)),
      m_isLive(false) {
  addWidget(m_pPlaceholderWidget);
}

void DebugImageView::setLive(const bool live) {
  if (live && !m_isLive) {
    ImageViewBase::backgroundExecutor().enqueueTask(make_intrusive<ImageLoader>(this, m_file.get()));
  } else if (!live && m_isLive) {
    if (QWidget* wgt = currentWidget()) {
      if (wgt != m_pPlaceholderWidget) {
        removeWidget(wgt);
        delete wgt;
      }
    }
  }

  m_isLive = live;
}

void DebugImageView::imageLoaded(const QImage& image) {
  if (!m_isLive) {
    return;
  }

  if (currentWidget() == m_pPlaceholderWidget) {
    std::unique_ptr<QWidget> image_view;
    if (m_imageViewFactory.empty()) {
      image_view = std::make_unique<BasicImageView>(image);
    } else {
      image_view.reset(m_imageViewFactory(image));
    }
    setCurrentIndex(addWidget(image_view.release()));
  }
}
