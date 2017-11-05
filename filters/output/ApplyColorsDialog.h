/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef OUTPUT_APPLYCOLORSDIALOG_H_
#define OUTPUT_APPLYCOLORSDIALOG_H_

#include "ui_OutputApplyColorsDialog.h"
#include "PageId.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QDialog>
#include <set>

class PageSelectionAccessor;
class QButtonGroup;

namespace output {
    class ApplyColorsDialog : public QDialog, private Ui::OutputApplyColorsDialog {
    Q_OBJECT
    public:
        ApplyColorsDialog(QWidget* parent, PageId const& page_id, PageSelectionAccessor const& page_selection_accessor);

        virtual ~ApplyColorsDialog();

    signals:

        void accepted(std::set<PageId> const& pages);

    private slots:

        void onSubmit();

    private:
        PageSequence m_pages;
        std::set<PageId> m_selectedPages;
        PageId m_curPage;
        QButtonGroup* m_pScopeGroup;
    };
}  // namespace output
#endif // ifndef OUTPUT_APPLYCOLORSDIALOG_H_
