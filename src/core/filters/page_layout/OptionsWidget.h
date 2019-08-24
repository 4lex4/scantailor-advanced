// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_OPTIONSWIDGET_H_
#define SCANTAILOR_PAGE_LAYOUT_OPTIONSWIDGET_H_

#include <UnitsListener.h>
#include <QIcon>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include "Alignment.h"
#include "FilterOptionsWidget.h"
#include "Margins.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "intrusive_ptr.h"
#include "ui_OptionsWidget.h"

class QToolButton;
class ProjectPages;

namespace page_layout {
class Settings;

class OptionsWidget : public FilterOptionsWidget, public UnitsListener, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageInfo& pageInfo, const Margins& marginsMm, const Alignment& alignment);

  void postUpdateUI();

  bool leftRightLinked() const;

  bool topBottomLinked() const;

  const Margins& marginsMM() const;

  const Alignment& alignment() const;

  void onUnitsChanged(Units units) override;

 signals:

  void leftRightLinkToggled(bool linked);

  void topBottomLinkToggled(bool linked);

  void alignmentChanged(const Alignment& alignment);

  void marginsSetLocally(const Margins& marginsMm);

  void aggregateHardSizeChanged();

 public slots:

  void marginsSetExternally(const Margins& marginsMm);

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

  void updateAlignmentModeEnabled();

  QToolButton* getCheckedAlignmentButton() const;

  void setupUiConnections();

  void removeUiConnections();

  void setupIcons();

  intrusive_ptr<Settings> m_settings;
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

  std::list<QMetaObject::Connection> m_connectionList;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_OPTIONSWIDGET_H_
