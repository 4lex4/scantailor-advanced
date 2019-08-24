// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_APPLYCOLORSDIALOG_H_
#define SCANTAILOR_OUTPUT_APPLYCOLORSDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <set>
#include "PageId.h"
#include "PageSequence.h"
#include "intrusive_ptr.h"
#include "ui_ApplyColorsDialog.h"

class PageSelectionAccessor;

namespace output {
class ApplyColorsDialog : public QDialog, private Ui::ApplyColorsDialog {
  Q_OBJECT
 public:
  ApplyColorsDialog(QWidget* parent, const PageId& pageId, const PageSelectionAccessor& pageSelectionAccessor);

  ~ApplyColorsDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages);

 private slots:

  void onSubmit();

 private:
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  QButtonGroup* m_scopeGroup;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_APPLYCOLORSDIALOG_H_
