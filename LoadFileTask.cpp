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

#include "LoadFileTask.h"
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
      m_ptrThumbnailCache(std::move(thumbnail_cache)),
      m_imageId(page.imageId()),
      m_imageMetadata(page.metadata()),
      m_ptrPages(std::move(pages)),
      m_ptrNextTask(std::move(next_task)) {
  assert(m_ptrNextTask);
}

LoadFileTask::~LoadFileTask() = default;

FilterResultPtr LoadFileTask::operator()() {
  QImage image(ImageLoader::load(m_imageId));

  try {
    throwIfCancelled();

    if (image.isNull()) {
      return make_intrusive<ErrorResult>(m_imageId.filePath());
    } else {
      updateImageSizeIfChanged(image);
      overrideDpi(image);
      m_ptrThumbnailCache->ensureThumbnailExists(m_imageId, image);

      return m_ptrNextTask->process(*this, FilterData(image));
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
    m_ptrPages->updateImageMetadata(m_imageId, m_imageMetadata);
  }
}

void LoadFileTask::overrideDpi(QImage& image) const {
  // Beware: QImage will have a default DPI when loading
  // an image that doesn't specify one.
  const Dpm dpm(m_imageMetadata.dpi());
  image.setDotsPerMeterX(dpm.horizontal());
  image.setDotsPerMeterY(dpm.vertical());
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
        : ErrorWidget(text, fmt), m_ptrRelinkingDialogRequester(std::move(relinking_dialog_requester)) {}

   private:
    void linkActivated(const QString&) override { (*m_ptrRelinkingDialogRequester)(); }

    intrusive_ptr<AbstractCommand<void>> m_ptrRelinkingDialogRequester;
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
