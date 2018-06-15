/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include "ui_OutputChangeDewarpingDialog.h"

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
  Ui::OutputChangeDewarpingDialog ui;
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  DewarpingMode m_dewarpingMode;
  DewarpingOptions m_dewarpingOptions;
  QButtonGroup* m_pScopeGroup;
};
}  // namespace output
#endif  // ifndef OUTPUT_CHANGE_DEWARPING_DIALOG_H_
