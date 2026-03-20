// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ApplyMarginsDialog.h"

#include <cassert>

#include "PageSelectionAccessor.h"

namespace page_layout {
ApplyMarginsDialog::ApplyMarginsDialog(QWidget* parent,
                                       const PageId& curPage,
                                       const PageSelectionAccessor& pageSelectionAccessor)
    : QDialog(parent),
      m_pages(pageSelectionAccessor.allPages()),
      m_selectedPages(pageSelectionAccessor.selectedPages()),
      m_selectedRanges(pageSelectionAccessor.selectedRanges()),
      m_curPage(curPage),
      m_scopeGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(selectedPagesRB);
  m_scopeGroup->addButton(everyOtherRB);
  m_scopeGroup->addButton(thisEveryOtherRB);
  m_scopeGroup->addButton(everyOtherSelectedRB);

  if (m_selectedPages.size() <= 1) {
    selectedPagesRB->setEnabled(false);
    selectedPagesHint->setEnabled(false);
    everyOtherSelectedRB->setEnabled(false);
    everyOtherSelectedHint->setEnabled(false);
  }

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyMarginsDialog::~ApplyMarginsDialog() = default;

void ApplyMarginsDialog::onSubmit() {
  std::set<PageId> pages;

  const bool applyLeft = applyLeftCB->isChecked();
  const bool applyRight = applyRightCB->isChecked();
  const bool applyTop = applyTopCB->isChecked();
  const bool applyBottom = applyBottomCB->isChecked();

  // thisPageRB is intentionally not handled.
  if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (selectedPagesRB->isChecked()) {
    emit accepted(m_selectedPages, applyLeft, applyRight, applyTop, applyBottom);
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
    assert(m_selectedRanges.size() == 1);
    const PageRange& range = m_selectedRanges.front();
    range.selectEveryOther(m_curPage).swap(pages);
  }

  emit accepted(pages, applyLeft, applyRight, applyTop, applyBottom);
  accept();
}
}  // namespace page_layout
