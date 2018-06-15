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

#ifndef FIX_ORIENTATION_FILTER_H_
#define FIX_ORIENTATION_FILTER_H_

#include "AbstractFilter.h"
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageView.h"
#include "SafeDeletingQObjectPtr.h"
#include "intrusive_ptr.h"

class ImageId;
class PageSelectionAccessor;
class QString;
class QDomDocument;
class QDomElement;
class ImageSettings;

namespace page_split {
class Task;
class CacheDrivenTask;
}  // namespace page_split

namespace fix_orientation {
class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;

/**
 * \note All methods of this class except the destructor
 *       must be called from the GUI thread only.
 */
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
                                 intrusive_ptr<page_split::Task> next_task,
                                 bool batch_processing);

  intrusive_ptr<CacheDrivenTask> createCacheDrivenTask(intrusive_ptr<page_split::CacheDrivenTask> next_task);

  OptionsWidget* optionsWidget();

 private:
  void writeParams(QDomDocument& doc, QDomElement& filter_el, const ImageId& image_id, int numeric_id) const;

  void saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filter_el) const;

  void writeImageParams(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const;

  void loadImageSettings(const ProjectReader& reader, const QDomElement& image_settings_el);

  intrusive_ptr<Settings> m_ptrSettings;
  intrusive_ptr<ImageSettings> m_ptrImageSettings;
  SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
};
}  // namespace fix_orientation
#endif  // ifndef FIX_ORIENTATION_FILTER_H_
