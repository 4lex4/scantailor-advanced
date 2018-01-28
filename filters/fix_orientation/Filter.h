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

#ifndef FIX_ORIENTATION_FILTER_H_
#define FIX_ORIENTATION_FILTER_H_

#include "NonCopyable.h"
#include "AbstractFilter.h"
#include "PageView.h"
#include "FilterResult.h"
#include "intrusive_ptr.h"
#include "SafeDeletingQObjectPtr.h"

class PageId;
class ImageId;
class PageSelectionAccessor;
class QString;
class QDomDocument;
class QDomElement;

namespace page_split {
    class Task;
    class CacheDrivenTask;
}

namespace fix_orientation {
    class OptionsWidget;
    class Task;
    class CacheDrivenTask;
    class Settings;

/**
 * \note All methods of this class except the destructor
 *       must be called from the GUI thread only.
 */
    class Filter : public AbstractFilter {
    DECLARE_NON_COPYABLE(Filter)

    public:
        explicit Filter(PageSelectionAccessor const& page_selection_accessor);

        ~Filter() override;

        QString getName() const override;

        PageView getView() const override;

        void performRelinking(AbstractRelinker const& relinker) override;

        void preUpdateUI(FilterUiInterface* ui, PageId const&) override;

        QDomElement saveSettings(ProjectWriter const& writer, QDomDocument& doc) const override;

        void loadSettings(ProjectReader const& reader, QDomElement const& filters_el) override;

        void loadDefaultSettings(PageId const& page_id) override;

        intrusive_ptr<Task> createTask(PageId const& page_id,
                                       intrusive_ptr<page_split::Task> next_task,
                                       bool batch_processing);

        intrusive_ptr<CacheDrivenTask>
        createCacheDrivenTask(intrusive_ptr<page_split::CacheDrivenTask> next_task);

        OptionsWidget* optionsWidget();

    private:
        void
        writeImageSettings(QDomDocument& doc, QDomElement& filter_el, ImageId const& image_id, int numeric_id) const;

        intrusive_ptr<Settings> m_ptrSettings;
        SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
    };
}  // fix_orientation
#endif // ifndef FIX_ORIENTATION_FILTER_H_
