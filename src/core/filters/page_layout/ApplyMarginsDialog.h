// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_APPLYMARGINSDIALOG_H_
#define SCANTAILOR_PAGE_LAYOUT_APPLYMARGINSDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <set>

#include "PageId.h"
#include "PageRange.h"
#include "PageSequence.h"
#include "ui_ApplyMarginsDialog.h"

class PageSelectionAccessor;

namespace page_layout {
class ApplyMarginsDialog : public QDialog, private Ui::ApplyMarginsDialog {
  Q_OBJECT
 public:
  ApplyMarginsDialog(QWidget* parent, const PageId& curPage, const PageSelectionAccessor& pageSelectionAccessor);

  ~ApplyMarginsDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages, bool applyLeft, bool applyRight, bool applyTop, bool applyBottom);

 private slots:

  void onSubmit();

 private:
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  std::vector<PageRange> m_selectedRanges;
  PageId m_curPage;
  QButtonGroup* m_scopeGroup;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_APPLYMARGINSDIALOG_H_
