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

#include "OptionsWidget.h"
#include "ApplyDialog.h"
#include "Settings.h"
#include "ScopedIncDec.h"

#include <iostream>

namespace select_content {
    OptionsWidget::OptionsWidget(intrusive_ptr<Settings> const& settings,
                                 PageSelectionAccessor const& page_selection_accessor)
            : m_ptrSettings(settings),
              m_pageSelectionAccessor(page_selection_accessor) {
        setupUi(this);

        setupUiConnections();
    }

    OptionsWidget::~OptionsWidget() {
    }

    void OptionsWidget::preUpdateUI(PageId const& page_id) {
        removeUiConnections();

        m_pageId = page_id;

        contentDetectAutoBtn->setEnabled(false);
        contentDetectManualBtn->setEnabled(false);
        contentDetectDisableBtn->setEnabled(false);
        pageDetectAutoBtn->setEnabled(false);
        pageDetectManualBtn->setEnabled(false);
        pageDetectDisableBtn->setEnabled(false);

        pageDetectOptions->setVisible(pageDetectAutoBtn->isChecked());

        setupUiConnections();
    }

    void OptionsWidget::postUpdateUI(UiData const& ui_data) {
        removeUiConnections();

        m_uiData = ui_data;

        updateContentModeIndication(ui_data.contentDetectionMode());
        updatePageModeIndication(ui_data.pageDetectionMode());

        contentDetectAutoBtn->setEnabled(true);
        contentDetectManualBtn->setEnabled(true);
        contentDetectDisableBtn->setEnabled(true);
        pageDetectAutoBtn->setEnabled(true);
        pageDetectManualBtn->setEnabled(true);
        pageDetectDisableBtn->setEnabled(true);

        fineTuneBtn->setChecked(ui_data.isFineTuningCornersEnabled());
        pageDetectOptions->setVisible(pageDetectAutoBtn->isChecked());

        setupUiConnections();
    }

    void OptionsWidget::manualContentRectSet(QRectF const& content_rect) {
        m_uiData.setContentRect(content_rect);
        m_uiData.setContentDetectionMode(MODE_MANUAL);
        m_uiData.setContentDetectionEnabled(true);

        updateContentModeIndication(MODE_MANUAL);

        if (m_uiData.pageDetectionMode() == MODE_AUTO) {
            m_uiData.setPageDetectionMode(MODE_MANUAL);
            updatePageModeIndication(MODE_MANUAL);
        }

        commitCurrentParams();

        emit invalidateThumbnail(m_pageId);
    }

    void OptionsWidget::manualPageRectSet(QRectF const& page_rect) {
        m_uiData.setPageRect(page_rect);
        m_uiData.setPageDetectionMode(MODE_MANUAL);
        m_uiData.setPageDetectionEnabled(true);
        pageDetectOptions->setVisible(false);

        updatePageModeIndication(MODE_MANUAL);

        if (m_uiData.contentDetectionMode() == MODE_AUTO) {
            m_uiData.setContentDetectionMode(MODE_MANUAL);
            updateContentModeIndication(MODE_MANUAL);
        }

        commitCurrentParams();

        emit invalidateThumbnail(m_pageId);
    }

    void OptionsWidget::contentDetectAutoToggled() {
        m_uiData.setContentDetectionMode(MODE_AUTO);
        m_uiData.setContentDetectionEnabled(true);

        commitCurrentParams();
        emit reloadRequested();
    }

    void OptionsWidget::contentDetectManualToggled() {
        m_uiData.setContentDetectionMode(MODE_MANUAL);
        m_uiData.setContentDetectionEnabled(true);

        if (m_uiData.pageDetectionMode() == MODE_AUTO) {
            m_uiData.setPageDetectionMode(MODE_MANUAL);
            updatePageModeIndication(MODE_MANUAL);
        }

        commitCurrentParams();
    }

    void OptionsWidget::contentDetectDisableToggled() {
        m_uiData.setContentDetectionEnabled(false);
        commitCurrentParams();
        contentDetectAutoBtn->setChecked(false);
        contentDetectManualBtn->setChecked(false);
        contentDetectDisableBtn->setChecked(true);
        emit reloadRequested();
    }

    void OptionsWidget::pageDetectAutoToggled() {
        m_uiData.setPageDetectionMode(MODE_AUTO);
        m_uiData.setPageDetectionEnabled(true);
        pageDetectOptions->setVisible(true);
        commitCurrentParams();
        emit reloadRequested();
    }

    void OptionsWidget::pageDetectManualToggled() {
        bool need_reload = !m_uiData.isPageDetectionEnabled();

        m_uiData.setPageDetectionMode(MODE_MANUAL);
        m_uiData.setPageDetectionEnabled(true);
        pageDetectOptions->setVisible(false);

        if (m_uiData.contentDetectionMode() == MODE_AUTO) {
            m_uiData.setContentDetectionMode(MODE_MANUAL);
            updateContentModeIndication(MODE_MANUAL);
        }

        commitCurrentParams();
        if (need_reload) {
            emit reloadRequested();
        }
    }

    void OptionsWidget::pageDetectDisableToggled() {
        m_uiData.setPageDetectionEnabled(false);
        pageDetectOptions->setVisible(false);
        commitCurrentParams();
        pageDetectDisableBtn->setChecked(true);
        emit reloadRequested();
    }

    void OptionsWidget::fineTuningChanged(bool checked) {
        m_uiData.setFineTuneCornersEnabled(checked);
        commitCurrentParams();
        if (m_uiData.isPageDetectionEnabled()) {
            emit reloadRequested();
        }
    }

    void OptionsWidget::updateContentModeIndication(AutoManualMode const mode) {
        if (!m_uiData.isContentDetectionEnabled()) {
            contentDetectDisableBtn->setChecked(true);
        } else {
            if (mode == MODE_AUTO) {
                contentDetectAutoBtn->setChecked(true);
            } else {
                contentDetectManualBtn->setChecked(true);
            }
        }
    }

    void OptionsWidget::updatePageModeIndication(AutoManualMode const mode) {
        if (!m_uiData.isPageDetectionEnabled()) {
            pageDetectDisableBtn->setChecked(true);
        } else {
            if (mode == MODE_AUTO) {
                pageDetectAutoBtn->setChecked(true);
            } else {
                pageDetectManualBtn->setChecked(true);
            }
        }
    }

    void OptionsWidget::commitCurrentParams() {
        Dependencies deps(m_uiData.dependencies());
        if ((!m_uiData.isContentDetectionEnabled() || m_uiData.contentDetectionMode() == MODE_AUTO)
            || (!m_uiData.isPageDetectionEnabled() || m_uiData.pageDetectionMode() == MODE_AUTO)) {
            deps.invalidate();
        }

        Params params(
                m_uiData.contentRect(), m_uiData.contentSizeMM(), m_uiData.pageRect(),
                deps, m_uiData.contentDetectionMode(), m_uiData.pageDetectionMode(),
                m_uiData.isContentDetectionEnabled(), m_uiData.isPageDetectionEnabled(),
                m_uiData.isFineTuningCornersEnabled()
        );
        params.computeDeviation(m_ptrSettings->avg());
        m_ptrSettings->setPageParams(m_pageId, params);
    }

    void OptionsWidget::showApplyToDialog() {
        ApplyDialog* dialog = new ApplyDialog(
                this, m_pageId, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(
                dialog, SIGNAL(applySelection(std::set<PageId> const &, bool, bool)),
                this, SLOT(applySelection(std::set<PageId> const &, bool, bool))
        );
        dialog->show();
    }

    void OptionsWidget::applySelection(std::set<PageId> const& pages,
                                       bool const apply_content_box,
                                       bool const apply_page_box) {
        if (pages.empty()) {
            return;
        }

        Dependencies deps(m_uiData.dependencies());
        if ((!m_uiData.isContentDetectionEnabled() || m_uiData.contentDetectionMode() == MODE_AUTO)
            || (!m_uiData.isPageDetectionEnabled() || m_uiData.pageDetectionMode() == MODE_AUTO)) {
            deps.invalidate();
        }

        Params const params(
                m_uiData.contentRect(), m_uiData.contentSizeMM(), m_uiData.pageRect(),
                deps, m_uiData.contentDetectionMode(),
                m_uiData.pageDetectionMode(), m_uiData.isContentDetectionEnabled(),
                m_uiData.isPageDetectionEnabled(), m_uiData.isFineTuningCornersEnabled()
        );

        for (PageId const& page_id : pages) {
            Params new_params(params);

            std::unique_ptr<Params> old_params = m_ptrSettings->getPageParams(page_id);
            if (old_params.get()) {
                if (new_params.isContentDetectionEnabled() && (new_params.contentDetectionMode() == MODE_MANUAL)) {
                    if (!apply_content_box) {
                        new_params.setContentRect(old_params->contentRect());
                        new_params.setContentSizeMM(old_params->contentSizeMM());
                    }
                }
                if (new_params.isPageDetectionEnabled() && (new_params.pageDetectionMode() == MODE_MANUAL)) {
                    if (!apply_page_box) {
                        new_params.setPageRect(old_params->pageRect());
                    }
                }
            }

            m_ptrSettings->setPageParams(page_id, new_params);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        emit reloadRequested();
    } // OptionsWidget::applySelection

    void OptionsWidget::setupUiConnections() {
        connect(contentDetectAutoBtn, SIGNAL(pressed()), this, SLOT(contentDetectAutoToggled()));
        connect(contentDetectManualBtn, SIGNAL(pressed()), this, SLOT(contentDetectManualToggled()));
        connect(contentDetectDisableBtn, SIGNAL(pressed()), this, SLOT(contentDetectDisableToggled()));
        connect(pageDetectAutoBtn, SIGNAL(pressed()), this, SLOT(pageDetectAutoToggled()));
        connect(pageDetectManualBtn, SIGNAL(pressed()), this, SLOT(pageDetectManualToggled()));
        connect(pageDetectDisableBtn, SIGNAL(pressed()), this, SLOT(pageDetectDisableToggled()));
        connect(fineTuneBtn, SIGNAL(toggled(bool)), this, SLOT(fineTuningChanged(bool)));
        connect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
    }

    void OptionsWidget::removeUiConnections() {
        disconnect(contentDetectAutoBtn, SIGNAL(pressed()), this, SLOT(contentDetectAutoToggled()));
        disconnect(contentDetectManualBtn, SIGNAL(pressed()), this, SLOT(contentDetectManualToggled()));
        disconnect(contentDetectDisableBtn, SIGNAL(pressed()), this, SLOT(contentDetectDisableToggled()));
        disconnect(pageDetectAutoBtn, SIGNAL(pressed()), this, SLOT(pageDetectAutoToggled()));
        disconnect(pageDetectManualBtn, SIGNAL(pressed()), this, SLOT(pageDetectManualToggled()));
        disconnect(pageDetectDisableBtn, SIGNAL(pressed()), this, SLOT(pageDetectDisableToggled()));
        disconnect(fineTuneBtn, SIGNAL(toggled(bool)), this, SLOT(fineTuningChanged(bool)));
        disconnect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
    }


/*========================= OptionsWidget::UiData ======================*/

    OptionsWidget::UiData::UiData()
            : m_contentDetectionMode(MODE_AUTO),
              m_contentDetectionEnabled(true),
              m_pageDetectionEnabled(false),
              m_fineTuneCornersEnabled(false) {
    }

    OptionsWidget::UiData::~UiData() {
    }

    void OptionsWidget::UiData::setSizeCalc(PhysSizeCalc const& calc) {
        m_sizeCalc = calc;
    }

    void OptionsWidget::UiData::setContentRect(QRectF const& content_rect) {
        m_contentRect = content_rect;
    }

    QRectF const& OptionsWidget::UiData::contentRect() const {
        return m_contentRect;
    }

    void OptionsWidget::UiData::setPageRect(QRectF const& page_rect) {
        m_pageRect = page_rect;
    }

    QRectF const& OptionsWidget::UiData::pageRect() const {
        return m_pageRect;
    }

    QSizeF OptionsWidget::UiData::contentSizeMM() const {
        return m_sizeCalc.sizeMM(m_contentRect);
    }

    void OptionsWidget::UiData::setDependencies(Dependencies const& deps) {
        m_deps = deps;
    }

    Dependencies const& OptionsWidget::UiData::dependencies() const {
        return m_deps;
    }

    void OptionsWidget::UiData::setContentDetectionMode(AutoManualMode mode) {
        m_contentDetectionMode = mode;
    }

    AutoManualMode OptionsWidget::UiData::contentDetectionMode() const {
        return m_contentDetectionMode;
    }

    void OptionsWidget::UiData::setPageDetectionMode(AutoManualMode mode) {
        m_pageDetectionMode = mode;
    }

    AutoManualMode OptionsWidget::UiData::pageDetectionMode() const {
        return m_pageDetectionMode;
    }

    void OptionsWidget::UiData::setContentDetectionEnabled(bool detect) {
        m_contentDetectionEnabled = detect;
    }

    void OptionsWidget::UiData::setPageDetectionEnabled(bool detect) {
        m_pageDetectionEnabled = detect;
    }

    void OptionsWidget::UiData::setFineTuneCornersEnabled(bool fine_tune) {
        m_fineTuneCornersEnabled = fine_tune;
    }
}  // namespace select_content