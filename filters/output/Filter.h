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

#ifndef OUTPUT_FILTER_H_
#define OUTPUT_FILTER_H_

#include <QImage>
#include "AbstractFilter.h"
#include "FillZonePropFactory.h"
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageView.h"
#include "PictureZonePropFactory.h"
#include "SafeDeletingQObjectPtr.h"
#include "intrusive_ptr.h"

class PageSelectionAccessor;
class ThumbnailPixmapCache;
class OutputFileNameGenerator;
class QString;

namespace output {
class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;

class Filter : public AbstractFilter {
  DECLARE_NON_COPYABLE(Filter)

 public:
  explicit Filter(const PageSelectionAccessor& page_selection_accessor);

  ~Filter() override;

  QString getName() const override;

  PageView getView() const override;

  void performRelinking(const AbstractRelinker& relinker) override;

  void preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) override;

  QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const override;

  void loadSettings(const ProjectReader& reader, const QDomElement& filters_el) override;

  void loadDefaultSettings(const PageInfo& page_info) override;

  intrusive_ptr<Task> createTask(const PageId& page_id,
                                 intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                                 const OutputFileNameGenerator& out_file_name_gen,
                                 bool batch,
                                 bool debug);

  intrusive_ptr<CacheDrivenTask> createCacheDrivenTask(const OutputFileNameGenerator& out_file_name_gen);

  OptionsWidget* optionsWidget();

 private:
  void writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const;

  intrusive_ptr<Settings> m_ptrSettings;
  SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
  PictureZonePropFactory m_pictureZonePropFactory;
  FillZonePropFactory m_fillZonePropFactory;
};
}  // namespace output
#endif  // ifndef OUTPUT_FILTER_H_
