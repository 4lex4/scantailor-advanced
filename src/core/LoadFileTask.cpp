// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LoadFileTask.h"
#include <imageproc/Grayscale.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextDocument>
#include "AbstractFilter.h"
#include "Dpm.h"
#include "ErrorWidget.h"
#include "FilterData.h"
#include "FilterOptionsWidget.h"
#include "FilterUiInterface.h"
#include "ImageLoader.h"
#include "ProjectPages.h"
#include "ThumbnailPixmapCache.h"
#include "filters/fix_orientation/Task.h"

using namespace imageproc;

class LoadFileTask::ErrorResult : public FilterResult {
  Q_DECLARE_TR_FUNCTIONS(LoadFileTask)
 public:
  explicit ErrorResult(const QString& file_path);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return nullptr; }

 private:
  QString m_filePath;
  bool m_fileExists;
};


LoadFileTask::LoadFileTask(Type type,
                           const PageInfo& page,
                           intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                           intrusive_ptr<ProjectPages> pages,
                           intrusive_ptr<fix_orientation::Task> next_task)
    : BackgroundTask(type),
      m_thumbnailCache(std::move(thumbnail_cache)),
      m_imageId(page.imageId()),
      m_imageMetadata(page.metadata()),
      m_pages(std::move(pages)),
      m_nextTask(std::move(next_task)) {
  assert(m_nextTask);
}

LoadFileTask::~LoadFileTask() = default;

FilterResultPtr LoadFileTask::operator()() {
  QImage image = ImageLoader::load(m_imageId);

  try {
    throwIfCancelled();

    if (image.isNull()) {
      return make_intrusive<ErrorResult>(m_imageId.filePath());
    } else {
      convertToSupportedFormat(image);
      updateImageSizeIfChanged(image);
      overrideDpi(image);
      m_thumbnailCache->ensureThumbnailExists(m_imageId, image);

      return m_nextTask->process(*this, FilterData(image));
    }
  } catch (const CancelledException&) {
    return nullptr;
  }
}

void LoadFileTask::updateImageSizeIfChanged(const QImage& image) {
  // The user might just replace a file with another one.
  // In that case, we update its size that we store.
  // Note that we don't do the same about DPI, because
  // a DPI mismatch between the image and the stored value
  // may indicate that the DPI was overridden.
  // TODO: do something about DPIs when we have the ability
  // to change DPIs at any point in time (not just when
  // creating a project).
  if (image.size() != m_imageMetadata.size()) {
    m_imageMetadata.setSize(image.size());
    m_pages->updateImageMetadata(m_imageId, m_imageMetadata);
  }
}

void LoadFileTask::overrideDpi(QImage& image) const {
  // Beware: QImage will have a default DPI when loading
  // an image that doesn't specify one.
  const Dpm dpm(m_imageMetadata.dpi());
  image.setDotsPerMeterX(dpm.horizontal());
  image.setDotsPerMeterY(dpm.vertical());
}

void LoadFileTask::convertToSupportedFormat(QImage& image) const {
  if (((image.format() == QImage::Format_Indexed8) && !image.isGrayscale()) || (image.depth() > 8)) {
    const QImage::Format fmt = image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
    image = image.convertToFormat(fmt);
  } else {
    image = toGrayscale(image);
  }
}

/*======================= LoadFileTask::ErrorResult ======================*/

LoadFileTask::ErrorResult::ErrorResult(const QString& file_path)
    : m_filePath(QDir::toNativeSeparators(file_path)), m_fileExists(QFile::exists(file_path)) {}

void LoadFileTask::ErrorResult::updateUI(FilterUiInterface* ui) {
  class ErrWidget : public ErrorWidget {
   public:
    ErrWidget(intrusive_ptr<AbstractCommand<void>> relinking_dialog_requester,
              const QString& text,
              Qt::TextFormat fmt = Qt::AutoText)
        : ErrorWidget(text, fmt), m_relinkingDialogRequester(std::move(relinking_dialog_requester)) {}

   private:
    void linkActivated(const QString&) override { (*m_relinkingDialogRequester)(); }

    intrusive_ptr<AbstractCommand<void>> m_relinkingDialogRequester;
  };


  QString err_msg;
  Qt::TextFormat fmt = Qt::AutoText;
  if (m_fileExists) {
    err_msg = tr("The following file could not be loaded:\n%1").arg(m_filePath);
    fmt = Qt::PlainText;
  } else {
    err_msg = tr("The following file doesn't exist:<br>%1<br>"
                 "<br>"
                 "Use the <a href=\"#relink\">Relinking Tool</a> to locate it.")
                  .arg(m_filePath.toHtmlEscaped());
    fmt = Qt::RichText;
  }
  ui->setImageWidget(new ErrWidget(ui->relinkingDialogRequester(), err_msg, fmt), ui->TRANSFER_OWNERSHIP);
  ui->setOptionsWidget(new FilterOptionsWidget, ui->TRANSFER_OWNERSHIP);
}  // LoadFileTask::ErrorResult::updateUI
