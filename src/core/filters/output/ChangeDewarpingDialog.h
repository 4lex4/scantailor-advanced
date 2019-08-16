// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_CHANGE_DEWARPING_DIALOG_H_
#define OUTPUT_CHANGE_DEWARPING_DIALOG_H_

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
                        const PageId& cur_page,
                        const DewarpingOptions& dewarpingOptions,
                        const PageSelectionAccessor& page_selection_accessor);

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
#endif  // ifndef OUTPUT_CHANGE_DEWARPING_DIALOG_H_
