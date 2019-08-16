// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ApplyColorsDialog.h"
#include "PageSelectionAccessor.h"

namespace output {
ApplyColorsDialog::ApplyColorsDialog(QWidget* parent,
                                     const PageId& cur_page,
                                     const PageSelectionAccessor& page_selection_accessor)
    : QDialog(parent),
      m_pages(page_selection_accessor.allPages()),
      m_selectedPages(page_selection_accessor.selectedPages()),
      m_curPage(cur_page),
      m_scopeGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(selectedPagesRB);
  if (m_selectedPages.size() <= 1) {
    selectedPagesRB->setEnabled(false);
    selectedPagesHint->setEnabled(false);
  }

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyColorsDialog::~ApplyColorsDialog() = default;

void ApplyColorsDialog::onSubmit() {
  std::set<PageId> pages;

  // thisPageRB is intentionally not handled.
  if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (selectedPagesRB->isChecked()) {
    emit accepted(m_selectedPages);
    accept();

    return;
  }

  emit accepted(pages);

  // We assume the default connection from accepted() to accept()
  // was removed.
  accept();
}
}  // namespace output