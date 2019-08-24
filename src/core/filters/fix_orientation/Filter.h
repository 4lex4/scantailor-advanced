// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_FILTER_H_
#define SCANTAILOR_FIX_ORIENTATION_FILTER_H_

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
  explicit Filter(const PageSelectionAccessor& pageSelectionAccessor);

  ~Filter() override;

  QString getName() const override;

  PageView getView() const override;

  void performRelinking(const AbstractRelinker& relinker) override;

  void preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) override;

  QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const override;

  void loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) override;

  void loadDefaultSettings(const PageInfo& pageInfo) override;

  intrusive_ptr<Task> createTask(const PageId& pageId, intrusive_ptr<page_split::Task> nextTask, bool batchProcessing);

  intrusive_ptr<CacheDrivenTask> createCacheDrivenTask(intrusive_ptr<page_split::CacheDrivenTask> nextTask);

  OptionsWidget* optionsWidget();

 private:
  void writeParams(QDomDocument& doc, QDomElement& filterEl, const ImageId& imageId, int numericId) const;

  void saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filterEl) const;

  void writeImageParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

  void loadImageSettings(const ProjectReader& reader, const QDomElement& imageSettingsEl);

  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  SafeDeletingQObjectPtr<OptionsWidget> m_optionsWidget;
};
}  // namespace fix_orientation
#endif  // ifndef SCANTAILOR_FIX_ORIENTATION_FILTER_H_
