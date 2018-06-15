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

#ifndef SELECT_CONTENT_FILTER_H_
#define SELECT_CONTENT_FILTER_H_

#include <QCoreApplication>
#include <vector>
#include "AbstractFilter.h"
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageOrderOption.h"
#include "PageView.h"
#include "SafeDeletingQObjectPtr.h"
#include "Settings.h"
#include "intrusive_ptr.h"

class PageSelectionAccessor;
class QString;

namespace page_layout {
class Task;
class CacheDrivenTask;
}  // namespace page_layout

namespace select_content {
class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;

class Filter : public AbstractFilter {
  DECLARE_NON_COPYABLE(Filter)

  Q_DECLARE_TR_FUNCTIONS(select_content::Filter)
 public:
  explicit Filter(const PageSelectionAccessor& page_selection_accessor);

  ~Filter() override;

  QString getName() const override;

  PageView getView() const override;

  int selectedPageOrder() const override;

  void selectPageOrder(int option) override;

  virtual std::vector<PageOrderOption> pageOrderOptions() const;

  void performRelinking(const AbstractRelinker& relinker) override;

  void preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) override;

  QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const override;

  void loadSettings(const ProjectReader& reader, const QDomElement& filters_el) override;

  void loadDefaultSettings(const PageInfo& page_info) override;

  intrusive_ptr<Task> createTask(const PageId& page_id,
                                 intrusive_ptr<page_layout::Task> next_task,
                                 bool batch,
                                 bool debug);

  intrusive_ptr<CacheDrivenTask> createCacheDrivenTask(intrusive_ptr<page_layout::CacheDrivenTask> next_task);

  OptionsWidget* optionsWidget();

 private:
  void writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const;


  intrusive_ptr<Settings> m_ptrSettings;
  SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
  std::vector<PageOrderOption> m_pageOrderOptions;
  int m_selectedPageOrder;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_FILTER_H_
