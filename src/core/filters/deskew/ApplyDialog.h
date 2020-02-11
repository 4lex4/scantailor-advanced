// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_APPLYDIALOG_H_
#define SCANTAILOR_DESKEW_APPLYDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <memory>
#include <set>

#include "PageId.h"
#include "PageSequence.h"
#include "ui_ApplyDialog.h"

class PageSelectionAccessor;

namespace deskew {
class ApplyDialog : public QDialog, private Ui::ApplyDialog {
  Q_OBJECT
 public:
  ApplyDialog(QWidget* parent, const PageId& curPage, const PageSelectionAccessor& pageSelectionAccessor);

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
#endif  // ifndef SCANTAILOR_DESKEW_APPLYDIALOG_H_
