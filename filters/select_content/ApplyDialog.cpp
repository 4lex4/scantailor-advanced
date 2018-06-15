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

#include "ApplyDialog.h"
#include <cassert>
#include "PageSelectionAccessor.h"

namespace select_content {
ApplyDialog::ApplyDialog(QWidget* parent, const PageId& cur_page, const PageSelectionAccessor& page_selection_accessor)
    : QDialog(parent),
      m_pages(page_selection_accessor.allPages()),
      m_selectedPages(page_selection_accessor.selectedPages()),
      m_selectedRanges(page_selection_accessor.selectedRanges()),
      m_curPage(cur_page),
      m_pBtnGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_pBtnGroup->addButton(thisPageOnlyRB);
  m_pBtnGroup->addButton(allPagesRB);
  m_pBtnGroup->addButton(thisPageAndFollowersRB);
  m_pBtnGroup->addButton(selectedPagesRB);
  m_pBtnGroup->addButton(everyOtherRB);
  m_pBtnGroup->addButton(thisEveryOtherRB);
  m_pBtnGroup->addButton(everyOtherSelectedRB);

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

  // thisPageOnlyRB is intentionally not handled.
  if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (selectedPagesRB->isChecked()) {
    m_selectedPages.swap(pages);
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

  emit applySelection(pages, applyContentBoxOption->isChecked(), applyPageBoxOption->isChecked());
  // We assume the default connection from accept() to accepted() was removed.
  accept();
}  // ApplyDialog::onSubmit
}  // namespace select_content