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

#include "ApplyDialog.h"
#include <iostream>
#include "PageSelectionAccessor.h"

namespace deskew {
ApplyDialog::ApplyDialog(QWidget* parent, const PageId& cur_page, const PageSelectionAccessor& page_selection_accessor)
    : QDialog(parent),
      m_pages(page_selection_accessor.allPages()),
      m_curPage(cur_page),
      m_selectedPages(page_selection_accessor.selectedPages()),
      m_pScopeGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_pScopeGroup->addButton(thisPageRB);
  m_pScopeGroup->addButton(allPagesRB);
  m_pScopeGroup->addButton(thisPageAndFollowersRB);
  m_pScopeGroup->addButton(everyOtherRB);
  m_pScopeGroup->addButton(thisEveryOtherRB);
  m_pScopeGroup->addButton(selectedPagesRB);
  m_pScopeGroup->addButton(everyOtherSelectedRB);
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