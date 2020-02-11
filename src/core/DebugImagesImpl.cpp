// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DebugImagesImpl.h"

#include <BinaryImage.h>

#include <QDir>
#include <QImage>
#include <QImageWriter>
#include <QTemporaryFile>

void DebugImagesImpl::add(const QImage& image,
                          const QString& label,
                          const boost::function<QWidget*(const QImage&)>& imageViewFactory) {
  QTemporaryFile file(QDir::tempPath() + "/scantailor-dbg-XXXXXX.png");
  if (!file.open()) {
    return;
  }

  AutoRemovingFile aremFile(file.fileName());
  file.setAutoRemove(false);

  QImageWriter writer(&file, "png");
  writer.setCompression(2);  // Trade space for speed.
  if (!writer.write(image)) {
    return;
  }

  m_sequence.push_back(std::make_shared<Item>(aremFile, label, imageViewFactory));
}

void DebugImagesImpl::add(const imageproc::BinaryImage& image,
                          const QString& label,
                          const boost::function<QWidget*(const QImage&)>& imageViewFactory) {
  add(image.toQImage(), label, imageViewFactory);
}

AutoRemovingFile DebugImagesImpl::retrieveNext(QString* label,
                                               boost::function<QWidget*(const QImage&)>* imageViewFactory) {
  if (m_sequence.empty()) {
    return AutoRemovingFile();
  }

  AutoRemovingFile file(m_sequence.front()->file);
  if (label) {
    *label = m_sequence.front()->label;
  }
  if (imageViewFactory) {
    *imageViewFactory = m_sequence.front()->imageViewFactory;
  }

  m_sequence.pop_front();
  return file;
}
