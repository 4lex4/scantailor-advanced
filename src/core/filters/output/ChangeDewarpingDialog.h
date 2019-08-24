// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_CHANGEDEWARPINGDIALOG_H_
#define SCANTAILOR_OUTPUT_CHANGEDEWARPINGDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <QString>
#include <set>
#include "DewarpingOptions.h"
#include "PageId.h"
#include "PageSequence.h"
#include "intrusive_ptr.h"
#include "ui_ChangeDewarpingDialog.h"

class PageSelectionAccessor;

namespace output {
class ChangeDewarpingDialog : public QDialog {
  Q_OBJECT
 public:
  ChangeDewarpingDialog(QWidget* parent,
                        const PageId& curPage,
                        const DewarpingOptions& dewarpingOptions,
                        const PageSelectionAccessor& pageSelectionAccessor);

  ~ChangeDewarpingDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages, const DewarpingOptions& dewarpingOptions);

 private slots:

  void onSubmit();

 private:
  Ui::ChangeDewarpingDialog ui;
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  DewarpingMode m_dewarpingMode;
  DewarpingOptions m_dewarpingOptions;
  QButtonGroup* m_scopeGroup;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_CHANGEDEWARPINGDIALOG_H_
