// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SplitModeDialog.h"
#include <cassert>
#include <iostream>
#include <core/IconProvider.h>
#include "PageSelectionAccessor.h"

namespace page_split {
SplitModeDialog::SplitModeDialog(QWidget* const parent,
                                 const PageId& cur_page,
                                 const PageSelectionAccessor& page_selection_accessor,
                                 const LayoutType layout_type,
                                 const PageLayout::Type auto_detected_layout_type)
    : QDialog(parent),
      m_pages(page_selection_accessor.allPages()),
      m_selectedPages(page_selection_accessor.selectedPages()),
      m_curPage(cur_page),
      m_scopeGroup(new QButtonGroup(this)),
      m_layoutType(layout_type),
      m_autoDetectedLayoutType(auto_detected_layout_type) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(thisEveryOtherRB);
  m_scopeGroup->addButton(everyOtherRB);
  m_scopeGroup->addButton(selectedPagesRB);
  m_scopeGroup->addButton(everyOtherSelectedRB);
  if (m_selectedPages.size() <= 1) {
    selectedPagesRB->setEnabled(false);
    selectedPagesHint->setEnabled(false);
    everyOtherSelectedRB->setEnabled(false);
    everyOtherSelectedHint->setEnabled(false);
  }

  layoutTypeLabel->setPixmap(iconFor(m_layoutType).pixmap(32, 32));
  if (m_layoutType == AUTO_LAYOUT_TYPE) {
    modeAuto->setChecked(true);
    applyCutOption->setEnabled(false);
  } else {
    modeManual->setChecked(true);
    applyCutOption->setEnabled(true);
  }

  connect(modeAuto, SIGNAL(pressed()), this, SLOT(autoDetectionSelected()));
  connect(modeManual, SIGNAL(pressed()), this, SLOT(manualModeSelected()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

SplitModeDialog::~SplitModeDialog() = default;

void SplitModeDialog::autoDetectionSelected() {
  layoutTypeLabel->setPixmap(iconFor(AUTO_LAYOUT_TYPE).pixmap(32, 32));
  applyCutOption->setEnabled(false);
  applyCutOption->setChecked(false);
}

void SplitModeDialog::manualModeSelected() {
  layoutTypeLabel->setPixmap(iconFor(combinedLayoutType()).pixmap(32, 32));
  applyCutOption->setEnabled(true);
}

void SplitModeDialog::onSubmit() {
  LayoutType layout_type = AUTO_LAYOUT_TYPE;
  if (modeManual->isChecked()) {
    layout_type = combinedLayoutType();
  }

  std::set<PageId> pages;

  if (thisPageRB->isChecked()) {
    pages.insert(m_curPage);
  } else if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (selectedPagesRB->isChecked()) {
    emit accepted(m_selectedPages, layout_type, applyCutOption->isChecked());
    accept();

    return;
  } else if (everyOtherRB->isChecked()) {
    m_pages.selectEveryOther(m_curPage).swap(pages);
  } else if (thisEveryOtherRB->isChecked()) {
    std::set<PageId> tmp;
    m_pages.selectPagePlusFollowers(m_curPage).swap(tmp);
    auto it = tmp.begin();
    for (int i = 0; it != tmp.end(); ++it, ++i) {
      if (i % 2 == 0) {
        pages.insert(*it);
      }
    }
  } else if (everyOtherSelectedRB->isChecked()) {
    auto it = m_selectedPages.begin();
    for (int i = 0; it != m_selectedPages.end(); ++it, ++i) {
      if (i % 2 == 0) {
        pages.insert(*it);
      }
    }
  }

  emit accepted(pages, layout_type, applyCutOption->isChecked());
  // We assume the default connection from accepted() to accept()
  // was removed.
  accept();
}  // SplitModeDialog::onSubmit

LayoutType SplitModeDialog::combinedLayoutType() const {
  if (m_layoutType != AUTO_LAYOUT_TYPE) {
    return m_layoutType;
  }

  switch (m_autoDetectedLayoutType) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      return SINGLE_PAGE_UNCUT;
    case PageLayout::SINGLE_PAGE_CUT:
      return PAGE_PLUS_OFFCUT;
    case PageLayout::TWO_PAGES:
      return TWO_PAGES;
  }

  assert(!"Unreachable");

  return AUTO_LAYOUT_TYPE;
}

QIcon SplitModeDialog::iconFor(const LayoutType layout_type) {
  QIcon icon;
  switch (layout_type) {
    case AUTO_LAYOUT_TYPE:
      icon = IconProvider::getInstance().getIcon("layout_type_auto");
      break;
    case SINGLE_PAGE_UNCUT:
      icon = IconProvider::getInstance().getIcon("single_page_uncut");
      break;
    case PAGE_PLUS_OFFCUT:
      icon = IconProvider::getInstance().getIcon("right_page_plus_offcut");
      break;
    case TWO_PAGES:
      icon = IconProvider::getInstance().getIcon("two_pages");
      break;
  }
  return icon;
}
}  // namespace page_split