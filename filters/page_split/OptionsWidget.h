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

#ifndef PAGE_SPLIT_OPTIONSWIDGET_H_
#define PAGE_SPLIT_OPTIONSWIDGET_H_

#include <set>
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "FilterOptionsWidget.h"
#include "ImageId.h"
#include "LayoutType.h"
#include "PageId.h"
#include "PageLayout.h"
#include "PageSelectionAccessor.h"
#include "intrusive_ptr.h"
#include "ui_PageSplitOptionsWidget.h"

class ProjectPages;

namespace page_split {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::PageSplitOptionsWidget {
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


  OptionsWidget(intrusive_ptr<Settings> settings,
                intrusive_ptr<ProjectPages> page_sequence,
                const PageSelectionAccessor& page_selection_accessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageId& page_id);

  void postUpdateUI(const UiData& ui_data);

 signals:

  void pageLayoutSetLocally(const PageLayout& page_layout);

 public slots:

  void pageLayoutSetExternally(const PageLayout& page_layout);

 private slots:

  void layoutTypeButtonToggled(bool checked);

  void showChangeDialog();

  void layoutTypeSet(const std::set<PageId>& pages, LayoutType layout_type, bool apply_cut);

  void splitLineModeChanged(bool auto_mode);

 private:
  void commitCurrentParams();

  void setupUiConnections();

  void removeUiConnections();

  intrusive_ptr<Settings> m_ptrSettings;
  intrusive_ptr<ProjectPages> m_ptrPages;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  UiData m_uiData;
  int m_ignoreAutoManualToggle;
  int m_ignoreLayoutTypeToggle;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_OPTIONSWIDGET_H_
