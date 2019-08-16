// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ApplyDialog.h"
#include <iostream>
#include "PageSelectionAccessor.h"

namespace deskew {
ApplyDialog::ApplyDialog(QWidget* parent, const PageId& cur_page, const PageSelectionAccessor& page_selection_accessor)
    : QDialog(parent),
      m_pages(page_selection_accessor.allPages()),
      m_curPage(cur_page),
      m_selectedPages(page_selection_accessor.selectedPages()),
      m_scopeGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(everyOtherRB);
  m_scopeGroup->addButton(thisEveryOtherRB);
  m_scopeGroup->addButton(selectedPagesRB);
  m_scopeGroup->addButton(everyOtherSelectedRB);
  if (m_selectedPages.size() <= 1) {
    selectedPagesRB->setEnabled(false);
    selectedPagesHint->setEnabled(false);
    everyOtherSelectedRB->setEnabled(false);
    everyOtherSelectedHint->setEnabled(false);
  }

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyDialog::~ApplyDialog() = default;

void ApplyDialog::onSubmit() {
  std::set<PageId> pages;
  // thisPageRB is intentionally not handled.
  if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
    emit appliedToAllPages(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
    emit appliedTo(pages);
  } else if (selectedPagesRB->isChecked()) {
    emit appliedTo(m_selectedPages);
  } else if (everyOtherRB->isChecked()) {
    m_pages.selectEveryOther(m_curPage).swap(pages);
    emit appliedTo(pages);
  } else if (thisEveryOtherRB->isChecked()) {
    std::set<PageId> tmp;
    m_pages.selectPagePlusFollowers(m_curPage).swap(tmp);
    auto it = tmp.begin();
    for (int i = 0; it != tmp.end(); ++it, ++i) {
      if (i % 2 == 0) {
        pages.insert(*it);
      }
    }
    emit appliedTo(pages);
  } else if (everyOtherSelectedRB->isChecked()) {
    auto it = m_selectedPages.begin();
    for (int i = 0; it != m_selectedPages.end(); ++it, ++i) {
      if (i % 2 == 0) {
        pages.insert(*it);
      }
    }
    emit appliedTo(pages);
  }
  accept();
}  // ApplyDialog::onSubmit
}  // namespace deskew