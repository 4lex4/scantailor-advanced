/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>
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

#ifndef DESKEW_APPLYDIALOG_H_
#define DESKEW_APPLYDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <set>
#include "PageId.h"
#include "PageSequence.h"
#include "intrusive_ptr.h"
#include "ui_DeskewApplyDialog.h"

class PageSelectionAccessor;

namespace deskew {
class ApplyDialog : public QDialog, private Ui::DeskewApplyDialog {
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
  QButtonGroup* m_pScopeGroup;
};
}  // namespace deskew
#endif  // ifndef DESKEW_APPLYDIALOG_H_
