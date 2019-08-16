// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DESKEW_APPLYDIALOG_H_
#define DESKEW_APPLYDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <set>
#include "PageId.h"
#include "PageSequence.h"
#include "intrusive_ptr.h"
#include "ui_ApplyDialog.h"

class PageSelectionAccessor;

namespace deskew {
class ApplyDialog : public QDialog, private Ui::ApplyDialog {
  Q_OBJECT
 public:
  ApplyDialog(QWidget* parent, const PageId& cur_page, const PageSelectionAccessor& page_selection_accessor);

  ~ApplyDialog() override;

 signals:

  void appliedTo(const std::set<PageId>& pages);

  void appliedToAllPages(const std::set<PageId>& pages);

 private slots:

  void onSubmit();

 private:
  PageSequence m_pages;
  PageId m_curPage;
  std::set<PageId> m_selectedPages;
  QButtonGroup* m_scopeGroup;
};
}  // namespace deskew
#endif  // ifndef DESKEW_APPLYDIALOG_H_
