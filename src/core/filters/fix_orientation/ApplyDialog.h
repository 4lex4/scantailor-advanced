// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_APPLYDIALOG_H_
#define SCANTAILOR_FIX_ORIENTATION_APPLYDIALOG_H_

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

namespace fix_orientation {
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
  std::set<PageId> m_selectedPages;
  std::vector<PageRange> m_selectedRanges;
  PageId m_curPage;
  QButtonGroup* m_btnGroup;
};
}  // namespace fix_orientation
#endif  // ifndef SCANTAILOR_FIX_ORIENTATION_APPLYDIALOG_H_
