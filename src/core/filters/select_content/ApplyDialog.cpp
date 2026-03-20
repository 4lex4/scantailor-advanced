// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ApplyDialog.h"

#include <cassert>

#include "PageSelectionAccessor.h"

namespace select_content {
ApplyDialog::ApplyDialog(QWidget* parent, const PageId& curPage, const PageSelectionAccessor& pageSelectionAccessor)
    : QDialog(parent),
      m_pages(pageSelectionAccessor.allPages()),
      m_selectedPages(pageSelectionAccessor.selectedPages()),
      m_selectedRanges(pageSelectionAccessor.selectedRanges()),
      m_curPage(curPage),
      m_btnGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_btnGroup->addButton(thisPageOnlyRB);
  m_btnGroup->addButton(allPagesRB);
  m_btnGroup->addButton(thisPageAndFollowersRB);
  m_btnGroup->addButton(selectedPagesRB);
  m_btnGroup->addButton(everyOtherRB);
  m_btnGroup->addButton(thisEveryOtherRB);
  m_btnGroup->addButton(everyOtherSelectedRB);

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
    // "This page and the following every other" in document order (issue #84).
    const int baseIdx = m_pages.pageNo(m_curPage);
    if (baseIdx >= 0) {
      for (size_t i = static_cast<size_t>(baseIdx); i < m_pages.numPages(); i += 2) {
        pages.insert(m_pages.pageAt(i).id());
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