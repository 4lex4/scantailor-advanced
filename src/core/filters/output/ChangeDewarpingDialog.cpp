// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ChangeDewarpingDialog.h"
#include <boost/lambda/lambda.hpp>
#include "PageSelectionAccessor.h"

namespace output {
ChangeDewarpingDialog::ChangeDewarpingDialog(QWidget* parent,
                                             const PageId& curPage,
                                             const DewarpingOptions& dewarpingOptions,
                                             const PageSelectionAccessor& pageSelectionAccessor)
    : QDialog(parent),
      m_pages(pageSelectionAccessor.allPages()),
      m_selectedPages(pageSelectionAccessor.selectedPages()),
      m_curPage(curPage),
      m_dewarpingMode(dewarpingOptions.dewarpingMode()),
      m_dewarpingOptions(dewarpingOptions),
      m_scopeGroup(new QButtonGroup(this)) {
  using namespace boost::lambda;

  ui.setupUi(this);
  m_scopeGroup->addButton(ui.thisPageRB);
  m_scopeGroup->addButton(ui.allPagesRB);
  m_scopeGroup->addButton(ui.thisPageAndFollowersRB);
  m_scopeGroup->addButton(ui.selectedPagesRB);
  if (m_selectedPages.size() <= 1) {
    ui.selectedPagesWidget->setEnabled(false);
  }

  switch (dewarpingOptions.dewarpingMode()) {
    case OFF:
      ui.offRB->setChecked(true);
      break;
    case AUTO:
      ui.autoRB->setChecked(true);
      break;
    case MARGINAL:
      ui.marginalRB->setChecked(true);
      break;
    case MANUAL:
      ui.manualRB->setChecked(true);
      break;
  }

  ui.dewarpingPostDeskewCB->setChecked(dewarpingOptions.needPostDeskew());
  connect(ui.offRB, &QRadioButton::clicked, var(m_dewarpingMode) = OFF);
  connect(ui.autoRB, &QRadioButton::clicked, var(m_dewarpingMode) = AUTO);
  connect(ui.manualRB, &QRadioButton::clicked, var(m_dewarpingMode) = MANUAL);
  connect(ui.marginalRB, &QRadioButton::clicked, var(m_dewarpingMode) = MARGINAL);

  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ChangeDewarpingDialog::~ChangeDewarpingDialog() = default;

void ChangeDewarpingDialog::onSubmit() {
  std::set<PageId> pages;

  m_dewarpingOptions.setDewarpingMode(m_dewarpingMode);
  m_dewarpingOptions.setPostDeskew(ui.dewarpingPostDeskewCB->isChecked());

  if (ui.thisPageRB->isChecked()) {
    pages.insert(m_curPage);
  } else if (ui.allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (ui.thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (ui.selectedPagesRB->isChecked()) {
    emit accepted(m_selectedPages, m_dewarpingOptions);
    accept();

    return;
  }

  emit accepted(pages, m_dewarpingOptions);

  // We assume the default connection from accepted() to accept()
  // was removed.
  accept();
}
}  // namespace output