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

#ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
#define SELECT_CONTENT_OPTIONSWIDGET_H_

#include "ui_SelectContentOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "intrusive_ptr.h"
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "PhysSizeCalc.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "Params.h"
#include <QSizeF>
#include <QRectF>
#include <memory>
#include <UnitsObserver.h>

namespace select_content {
    class Settings;

    class OptionsWidget
            : public FilterOptionsWidget, public UnitsObserver, private Ui::SelectContentOptionsWidget {
    Q_OBJECT
    public:
        class UiData {
            // Member-wise copying is OK.
        public:
            UiData();

            ~UiData();

            void setSizeCalc(PhysSizeCalc const& calc);

            void setContentRect(QRectF const& content_rect);

            void setPageRect(QRectF const& content_rect);

            QRectF const& contentRect() const;

            QRectF const& pageRect() const;

            QSizeF contentSizeMM() const;

            void setDependencies(Dependencies const& deps);

            Dependencies const& dependencies() const;

            void setContentDetectionMode(AutoManualMode mode);

            void setPageDetectionMode(AutoManualMode mode);

            bool isContentDetectionEnabled() const;

            bool isPageDetectionEnabled() const;

            bool isFineTuningCornersEnabled() const;

            void setContentDetectionEnabled(bool detect);

            void setPageDetectionEnabled(bool detect);

            void setFineTuneCornersEnabled(bool fine_tune);

            AutoManualMode contentDetectionMode() const;

            AutoManualMode pageDetectionMode() const;

        private:
            QRectF m_contentRect;  // In virtual image coordinates.
            QRectF m_pageRect;
            PhysSizeCalc m_sizeCalc;
            Dependencies m_deps;
            AutoManualMode m_contentDetectionMode;
            AutoManualMode m_pageDetectionMode;
            bool m_contentDetectionEnabled;
            bool m_pageDetectionEnabled;
            bool m_fineTuneCornersEnabled;
        };


        OptionsWidget(intrusive_ptr<Settings> settings, PageSelectionAccessor const& page_selection_accessor);

        ~OptionsWidget() override;

        void preUpdateUI(PageId const& page_id);

        void postUpdateUI(UiData const& ui_data);

        void updateUnits(Units units) override;

    public slots:

        void manualContentRectSet(QRectF const& content_rect);

        void manualPageRectSet(QRectF const& page_rect);

        void updatePageRectSize(QSizeF const& size);

    signals:

        void pageRectChangedLocally(QRectF const& pageRect);

    private slots:

        void showApplyToDialog();

        void applySelection(std::set<PageId> const& pages, bool apply_content_box, bool apply_page_box);

        void contentDetectAutoToggled();

        void contentDetectManualToggled();

        void contentDetectDisableToggled();

        void pageDetectAutoToggled();

        void pageDetectManualToggled();

        void pageDetectDisableToggled();

        void fineTuningChanged(bool checked);

        void dimensionsChangedLocally(double);

    private:
        void updateContentModeIndication(AutoManualMode mode);

        void updatePageModeIndication(AutoManualMode mode);

        void updatePageDetectOptionsDisplay();

        void commitCurrentParams();

        void setupUiConnections();

        void removeUiConnections();

        intrusive_ptr<Settings> m_ptrSettings;
        UiData m_uiData;
        PageSelectionAccessor m_pageSelectionAccessor;
        PageId m_pageId;
        int m_ignorePageSizeChanges;
    };
}  // namespace select_content
#endif // ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
