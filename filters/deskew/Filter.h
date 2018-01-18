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

class PageId;
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
        Filter(PageSelectionAccessor const& page_selection_accessor);

        virtual ~Filter();

        virtual QString getName() const;

        virtual PageView getView() const;

        virtual void performRelinking(AbstractRelinker const& relinker);

        virtual void preUpdateUI(FilterUiInterface* ui, PageId const& page_id);

        virtual void updateStatistics() {
            m_ptrSettings->updateDeviation();
        }

        virtual QDomElement saveSettings(ProjectWriter const& writer, QDomDocument& doc) const;

        virtual void loadSettings(ProjectReader const& reader, QDomElement const& filters_el);

        void loadDefaultSettings(PageId const& page_id) override;

        intrusive_ptr<Task> createTask(PageId const& page_id,
                                       intrusive_ptr<select_content::Task> const& next_task,
                                       bool batch_processing,
                                       bool debug);

        intrusive_ptr<CacheDrivenTask>
        createCacheDrivenTask(intrusive_ptr<select_content::CacheDrivenTask> const& next_task);

        OptionsWidget* optionsWidget() {
            return m_ptrOptionsWidget.get();
        }

        Settings* getSettings() {
            return m_ptrSettings.get();
        }

    private:
        void writePageSettings(QDomDocument& doc, QDomElement& filter_el, PageId const& page_id, int numeric_id) const;

        intrusive_ptr<Settings> m_ptrSettings;
        SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
    };
}  // namespace deskew
#endif  // ifndef DESKEW_FILTER_H_
