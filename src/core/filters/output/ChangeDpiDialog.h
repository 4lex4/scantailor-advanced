// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_CHANGEDPIDIALOG_H_
#define SCANTAILOR_OUTPUT_CHANGEDPIDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <QString>
#include <set>
#include "PageId.h"
#include "PageSequence.h"
#include "intrusive_ptr.h"
#include "ui_ChangeDpiDialog.h"

class PageSelectionAccessor;
class Dpi;

namespace output {
class ChangeDpiDialog : public QDialog, private Ui::ChangeDpiDialog {
  Q_OBJECT
 public:
  ChangeDpiDialog(QWidget* parent,
                  const Dpi& dpi,
                  const PageId& curPage,
                  const PageSelectionAccessor& pageSelectionAccessor);

  ~ChangeDpiDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages, const Dpi& dpi);

 private slots:

  void dpiSelectionChanged(int index);

  void dpiEditTextChanged(const QString& text);

  void onSubmit();

 private:
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  QButtonGroup* m_scopeGroup;
  int m_customItemIdx;
  QString m_customDpiString;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_CHANGEDPIDIALOG_H_
