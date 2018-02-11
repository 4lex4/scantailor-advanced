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

#ifndef DESKEW_FILTER_H_
#define DESKEW_FILTER_H_

#include "NonCopyable.h"
#include "AbstractFilter.h"
#include "PageView.h"
#include "intrusive_ptr.h"
#include "FilterResult.h"
#include "SafeDeletingQObjectPtr.h"
#include "Settings.h"

class QString;
class PageSelectionAccessor;

namespace select_content {
    class Task;
    class CacheDrivenTask;
}

namespace deskew {
    class OptionsWidget;
    class Task;
    class CacheDrivenTask;
    class Settings;

    class Filter : public AbstractFilter {
    DECLARE_NON_COPYABLE(Filter)

    public:
        explicit Filter(const PageSelectionAccessor& page_selection_accessor);

        ~Filter() override;

        QString getName() const override;

        PageView getView() const override;

        void performRelinking(const AbstractRelinker& relinker) override;

        void preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) override;

        void updateStatistics() override;

        QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const override;

        void loadSettings(const ProjectReader& reader, const QDomElement& filters_el) override;

        void loadDefaultSettings(const PageInfo& page_info) override;

        intrusive_ptr<Task> createTask(const PageId& page_id,
                                       intrusive_ptr<select_content::Task> next_task,
                                       bool batch_processing,
                                       bool debug);

        intrusive_ptr<CacheDrivenTask>
        createCacheDrivenTask(intrusive_ptr<select_content::CacheDrivenTask> next_task);

        OptionsWidget* optionsWidget();

    private:
        void writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const;

        intrusive_ptr<Settings> m_ptrSettings;
        SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
    };
}  // namespace deskew
#endif  // ifndef DESKEW_FILTER_H_
