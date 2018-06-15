/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PAGE_LAYOUT_OPTIONSWIDGET_H_
#define PAGE_LAYOUT_OPTIONSWIDGET_H_

#include <UnitsObserver.h>
#include <QIcon>
#include <memory>
#include <set>
#include <unordered_map>
#include "Alignment.h"
#include "FilterOptionsWidget.h"
#include "Margins.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "intrusive_ptr.h"
#include "ui_PageLayoutOptionsWidget.h"

class QToolButton;
class ProjectPages;

namespace page_layout {
class Settings;

class OptionsWidget : public FilterOptionsWidget, public UnitsObserver, private Ui::PageLayoutOptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageInfo& page_info, const Margins& margins_mm, const Alignment& alignment);

  void postUpdateUI();

  bool leftRightLinked() const;

  bool topBottomLinked() const;

  const Margins& marginsMM() const;

  const Alignment& alignment() const;

  void updateUnits(Units units) override;

 signals:

  void leftRightLinkToggled(bool linked);

  void topBottomLinkToggled(bool linked);

  void alignmentChanged(const Alignment& alignment);

  void marginsSetLocally(const Margins& margins_mm);

  void aggregateHardSizeChanged();

 public slots:

  void marginsSetExternally(const Margins& margins_mm);

 private slots:

  void horMarginsChanged(double val);

  void vertMarginsChanged(double val);

  void autoMarginsToggled(bool checked);

  void alignmentModeChanged(int idx);

  void topBottomLinkClicked();

  void leftRightLinkClicked();

  void alignWithOthersToggled();

  void alignmentButtonClicked();

  void autoHorizontalAligningToggled(bool checked);

  void autoVerticalAligningToggled(bool checked);

  void showApplyMarginsDialog();

  void showApplyAlignmentDialog();

  void applyMargins(const std::set<PageId>& pages);

  void applyAlignment(const std::set<PageId>& pages);

 private:
  typedef std::unordered_map<QToolButton*, Alignment> AlignmentByButton;

  void updateMarginsDisplay();

  void updateLinkDisplay(QToolButton* button, bool linked);

  void updateAlignmentButtonsEnabled();

  void updateMarginsControlsEnabled();

  void updateAutoModeButtons();

  QToolButton* getCheckedAlignmentButton() const;

  void setupUiConnections();

  void removeUiConnections();

  intrusive_ptr<Settings> m_ptrSettings;
  PageSelectionAccessor m_pageSelectionAccessor;
  QIcon m_chainIcon;
  QIcon m_brokenChainIcon;
  AlignmentByButton m_alignmentByButton;
  PageId m_pageId;
  Dpi m_dpi;
  Margins m_marginsMM;
  Alignment m_alignment;
  bool m_leftRightLinked;
  bool m_topBottomLinked;
  std::unique_ptr<QButtonGroup> m_alignmentButtonGroup;

  int m_ignoreMarginChanges = 0;
  int m_ignoreAlignmentButtonsChanges = 0;
};
}  // namespace page_layout
#endif  // ifndef PAGE_LAYOUT_OPTIONSWIDGET_H_
