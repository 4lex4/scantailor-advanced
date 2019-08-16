// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_FILTER_H_
#define OUTPUT_FILTER_H_

#include <QCoreApplication>
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

  Q_DECLARE_TR_FUNCTIONS(output::Filter)
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

  std::vector<PageOrderOption> pageOrderOptions() const override;

  int selectedPageOrder() const override;

  void selectPageOrder(int option) override;

 private:
  void writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const;

  intrusive_ptr<Settings> m_settings;
  SafeDeletingQObjectPtr<OptionsWidget> m_optionsWidget;
  PictureZonePropFactory m_pictureZonePropFactory;
  FillZonePropFactory m_fillZonePropFactory;
  std::vector<PageOrderOption> m_pageOrderOptions;
  int m_selectedPageOrder;
};
}  // namespace output
#endif  // ifndef OUTPUT_FILTER_H_
