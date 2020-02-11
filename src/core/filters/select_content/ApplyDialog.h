// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_APPLYDIALOG_H_
#define SCANTAILOR_SELECT_CONTENT_APPLYDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <memory>
#include <set>
#include <vector>

#include "PageId.h"
#include "PageRange.h"
#include "PageSequence.h"
#include "ui_ApplyDialog.h"

class PageSelectionAccessor;

namespace select_content {
class ApplyDialog : public QDialog, private Ui::ApplyDialog {
  Q_OBJECT
 public:
  ApplyDialog(QWidget* parent, const PageId& curPage, const PageSelectionAccessor& pageSelectionAccessor);

  ~ApplyDialog() override;

 signals:

  void applySelection(const std::set<PageId>& pages, bool applyContentBox, bool applyPageBox);

 private slots:

  void onSubmit();

 private:
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  std::vector<PageRange> m_selectedRanges;
  PageId m_curPage;
  QButtonGroup* m_btnGroup;
};
}  // namespace select_content
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_APPLYDIALOG_H_
