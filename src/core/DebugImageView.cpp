// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  ImageLoadResult(QPointer<DebugImageView> owner, const QImage& image) : m_owner(std::move(owner)), m_image(image) {}

  // This method is called from the main thread.
  void operator()() override {
    if (DebugImageView* owner = m_owner) {
      owner->imageLoaded(m_image);
    }
  }

 private:
  QPointer<DebugImageView> m_owner;
  QImage m_image;
};


class DebugImageView::ImageLoader : public AbstractCommand<BackgroundExecutor::TaskResultPtr> {
 public:
  ImageLoader(DebugImageView* owner, const QString& filePath) : m_owner(owner), m_filePath(filePath) {}

  BackgroundExecutor::TaskResultPtr operator()() override {
    QImage image(m_filePath);
    return make_intrusive<ImageLoadResult>(m_owner, image);
  }

 private:
  QPointer<DebugImageView> m_owner;
  QString m_filePath;
};


DebugImageView::DebugImageView(AutoRemovingFile file,
                               const boost::function<QWidget*(const QImage&)>& imageViewFactory,
                               QWidget* parent)
    : QStackedWidget(parent),
      m_file(file),
      m_imageViewFactory(imageViewFactory),
      m_placeholderWidget(new ProcessingIndicationWidget(this)),
      m_isLive(false) {
  addWidget(m_placeholderWidget);
}

void DebugImageView::setLive(const bool live) {
  if (live && !m_isLive) {
    ImageViewBase::backgroundExecutor().enqueueTask(make_intrusive<ImageLoader>(this, m_file.get()));
  } else if (!live && m_isLive) {
    if (QWidget* wgt = currentWidget()) {
      if (wgt != m_placeholderWidget) {
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

  if (currentWidget() == m_placeholderWidget) {
    std::unique_ptr<QWidget> imageView;
    if (m_imageViewFactory.empty()) {
      imageView = std::make_unique<BasicImageView>(image);
    } else {
      imageView.reset(m_imageViewFactory(image));
    }
    setCurrentIndex(addWidget(imageView.release()));
  }
}
