// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_OPTIONSWIDGET_H_
#define SCANTAILOR_PAGE_SPLIT_OPTIONSWIDGET_H_

#include <core/ConnectionManager.h>

#include <list>
#include <memory>
#include <set>

#include "AutoManualMode.h"
#include "Dependencies.h"
#include "FilterOptionsWidget.h"
#include "ImageId.h"
#include "LayoutType.h"
#include "PageId.h"
#include "PageLayout.h"
#include "PageSelectionAccessor.h"
#include "ui_OptionsWidget.h"

class ProjectPages;

namespace page_split {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  class UiData {
    // Member-wise copying is OK.
   public:
    UiData();

    ~UiData();

    void setPageLayout(const PageLayout& layout);

    const PageLayout& pageLayout() const;

    void setDependencies(const Dependencies& deps);

    const Dependencies& dependencies() const;

    void setSplitLineMode(AutoManualMode mode);

    AutoManualMode splitLineMode() const;

    bool layoutTypeAutoDetected() const;

    void setLayoutTypeAutoDetected(bool val);

   private:
    PageLayout m_pageLayout;
    Dependencies m_deps;
    AutoManualMode m_splitLineMode;
    bool m_layoutTypeAutoDetected;
  };


  OptionsWidget(std::shared_ptr<Settings> settings,
                std::shared_ptr<ProjectPages> pageSequence,
                const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageId& pageId);

  void postUpdateUI(const UiData& uiData);

 signals:

  void pageLayoutSetLocally(const PageLayout& pageLayout);

 public slots:

  void pageLayoutSetExternally(const PageLayout& pageLayout);

 private slots:

  void layoutTypeButtonToggled(bool checked);

  void showChangeDialog();

  void layoutTypeSet(const std::set<PageId>& pages, LayoutType layoutType, bool applyCut);

  void splitLineModeChanged(bool autoMode);

 private:
  void commitCurrentParams();

  void setupUiConnections();

  void setupIcons();

  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<ProjectPages> m_pages;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  UiData m_uiData;

  ConnectionManager m_connectionManager;
};


inline void OptionsWidget::UiData::setPageLayout(const PageLayout& layout) {
  m_pageLayout = layout;
}

inline const PageLayout& OptionsWidget::UiData::pageLayout() const {
  return m_pageLayout;
}

inline void OptionsWidget::UiData::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

inline const Dependencies& OptionsWidget::UiData::dependencies() const {
  return m_deps;
}

inline void OptionsWidget::UiData::setSplitLineMode(const AutoManualMode mode) {
  m_splitLineMode = mode;
}

inline AutoManualMode OptionsWidget::UiData::splitLineMode() const {
  return m_splitLineMode;
}

inline bool OptionsWidget::UiData::layoutTypeAutoDetected() const {
  return m_layoutTypeAutoDetected;
}

inline void OptionsWidget::UiData::setLayoutTypeAutoDetected(const bool val) {
  m_layoutTypeAutoDetected = val;
}
}  // namespace page_split
#endif  // ifndef SCANTAILOR_PAGE_SPLIT_OPTIONSWIDGET_H_
