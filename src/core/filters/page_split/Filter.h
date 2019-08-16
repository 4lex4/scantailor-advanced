// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_SPLIT_FILTER_H_
#define PAGE_SPLIT_FILTER_H_

#include <QCoreApplication>
#include <set>
#include "AbstractFilter.h"
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageOrderOption.h"
#include "PageView.h"
#include "SafeDeletingQObjectPtr.h"
#include "intrusive_ptr.h"

class ImageId;
class PageInfo;
class ProjectPages;
class PageSelectionAccessor;
class OrthogonalRotation;

namespace deskew {
class Task;
class CacheDrivenTask;
}  // namespace deskew

namespace page_split {
class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;

class Params;

class Filter : public AbstractFilter {
  DECLARE_NON_COPYABLE(Filter)

  Q_DECLARE_TR_FUNCTIONS(page_split::Filter)
 public:
  Filter(intrusive_ptr<ProjectPages> page_sequence, const PageSelectionAccessor& page_selection_accessor);

  ~Filter() override;

  QString getName() const override;

  PageView getView() const override;

  void performRelinking(const AbstractRelinker& relinker) override;

  void preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) override;

  QDomElement saveSettings(const ProjectWriter& wirter, QDomDocument& doc) const override;

  void loadSettings(const ProjectReader& reader, const QDomElement& filters_el) override;

  void loadDefaultSettings(const PageInfo& page_info) override;

  intrusive_ptr<Task> createTask(const PageInfo& page_info,
                                 intrusive_ptr<deskew::Task> next_task,
                                 bool batch_processing,
                                 bool debug);

  intrusive_ptr<CacheDrivenTask> createCacheDrivenTask(intrusive_ptr<deskew::CacheDrivenTask> next_task);

  OptionsWidget* optionsWidget();

  void pageOrientationUpdate(const ImageId& image_id, const OrthogonalRotation& orientation);

  std::vector<PageOrderOption> pageOrderOptions() const override;

  int selectedPageOrder() const override;

  void selectPageOrder(int option) override;

 private:
  void writeImageSettings(QDomDocument& doc, QDomElement& filter_el, const ImageId& image_id, int numeric_id) const;

  intrusive_ptr<ProjectPages> m_pages;
  intrusive_ptr<Settings> m_settings;
  SafeDeletingQObjectPtr<OptionsWidget> m_optionsWidget;
  std::vector<PageOrderOption> m_pageOrderOptions;
  int m_selectedPageOrder;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_FILTER_H_
