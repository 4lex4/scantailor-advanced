// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_FILTER_H_
#define SCANTAILOR_DESKEW_FILTER_H_

#include <QCoreApplication>
#include "AbstractFilter.h"
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageView.h"
#include "SafeDeletingQObjectPtr.h"
#include "Settings.h"
#include "intrusive_ptr.h"

class QString;
class PageSelectionAccessor;
class ImageSettings;

namespace select_content {
class Task;
class CacheDrivenTask;
}  // namespace select_content

namespace deskew {
class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;

class Filter : public AbstractFilter {
  DECLARE_NON_COPYABLE(Filter)

  Q_DECLARE_TR_FUNCTIONS(deskew::Filter)
 public:
  explicit Filter(const PageSelectionAccessor& pageSelectionAccessor);

  ~Filter() override;

  QString getName() const override;

  PageView getView() const override;

  void performRelinking(const AbstractRelinker& relinker) override;

  void preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) override;

  QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const override;

  void loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) override;

  void loadDefaultSettings(const PageInfo& pageInfo) override;

  intrusive_ptr<Task> createTask(const PageId& pageId,
                                 intrusive_ptr<select_content::Task> nextTask,
                                 bool batchProcessing,
                                 bool debug);

  intrusive_ptr<CacheDrivenTask> createCacheDrivenTask(intrusive_ptr<select_content::CacheDrivenTask> nextTask);

  OptionsWidget* optionsWidget();

  std::vector<PageOrderOption> pageOrderOptions() const override;

  int selectedPageOrder() const override;

  void selectPageOrder(int option) override;

 private:
  void writeParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

  void saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filterEl) const;

  void writeImageParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

  void loadImageSettings(const ProjectReader& reader, const QDomElement& imageSettingsEl);

  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  SafeDeletingQObjectPtr<OptionsWidget> m_optionsWidget;
  std::vector<PageOrderOption> m_pageOrderOptions;
  int m_selectedPageOrder;
};
}  // namespace deskew
#endif  // ifndef SCANTAILOR_DESKEW_FILTER_H_
