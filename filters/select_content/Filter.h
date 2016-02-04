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

#ifndef SELECT_CONTENT_FILTER_H_
#define SELECT_CONTENT_FILTER_H_

#include "NonCopyable.h"
#include "AbstractFilter.h"
#include "PageView.h"
#include "IntrusivePtr.h"
#include "FilterResult.h"
#include "SafeDeletingQObjectPtr.h"
#include "PageOrderOption.h"
#include "Settings.h"
#include <QCoreApplication>
#include <vector>

class PageId;

class PageSelectionAccessor;

class QString;

namespace page_layout
{
    class Task;

    class CacheDrivenTask;
}

namespace select_content
{

    class OptionsWidget;

    class Task;

    class CacheDrivenTask;

    class Settings;

    class Filter : public AbstractFilter
    {
    DECLARE_NON_COPYABLE(Filter)

    Q_DECLARE_TR_FUNCTIONS(select_content::Filter)
    public:
        Filter(PageSelectionAccessor const& page_selection_accessor);

        virtual ~Filter();

        virtual QString getName() const;

        virtual PageView getView() const;

        virtual int selectedPageOrder() const;

        virtual void selectPageOrder(int option);

        virtual std::vector<PageOrderOption> pageOrderOptions() const;

        virtual void performRelinking(AbstractRelinker const& relinker);

        virtual void preUpdateUI(FilterUiInterface* ui, PageId const& page_id);

        virtual void updateStatistics()
        { m_ptrSettings->updateDeviation(); }

        virtual QDomElement saveSettings(
                ProjectWriter const& writer, QDomDocument& doc) const;

        virtual void loadSettings(
                ProjectReader const& reader, QDomElement const& filters_el);

        IntrusivePtr<Task> createTask(
                PageId const& page_id,
                IntrusivePtr<page_layout::Task> const& next_task,
                bool batch, bool debug);

        IntrusivePtr<CacheDrivenTask> createCacheDrivenTask(
                IntrusivePtr<page_layout::CacheDrivenTask> const& next_task);

        OptionsWidget* optionsWidget()
        { return m_ptrOptionsWidget.get(); };

        Settings* getSettings()
        { return m_ptrSettings.get(); };
    private:
        void writePageSettings(
                QDomDocument& doc, QDomElement& filter_el,
                PageId const& page_id, int numeric_id) const;


        IntrusivePtr<Settings> m_ptrSettings;
        SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
        std::vector<PageOrderOption> m_pageOrderOptions;
        int m_selectedPageOrder;
    };

}
#endif
