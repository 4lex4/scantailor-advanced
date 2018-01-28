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
#include "ChangeDpiDialog.h"
#include "ChangeDewarpingDialog.h"
#include "ApplyColorsDialog.h"
#include "PictureZoneComparator.h"
#include "FillZoneComparator.h"
#include "OtsuBinarizationOptionsWidget.h"
#include "SauvolaBinarizationOptionsWidget.h"
#include "WolfBinarizationOptionsWidget.h"
#include "../../Utils.h"
#include <QToolTip>
#include <utility>

namespace output {
    OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings,
                                 PageSelectionAccessor const& page_selection_accessor)
            : m_ptrSettings(std::move(settings)),
              m_pageSelectionAccessor(page_selection_accessor),
              m_despeckleLevel(DESPECKLE_NORMAL),
              m_lastTab(TAB_OUTPUT) {
        setupUi(this);

        delayedReloadRequest.setSingleShot(true);

        depthPerceptionSlider->setMinimum(qRound(DepthPerception::minValue() * 10));
        depthPerceptionSlider->setMaximum(qRound(DepthPerception::maxValue() * 10));

        colorModeSelector->addItem(tr("Black and White"), BLACK_AND_WHITE);
        colorModeSelector->addItem(tr("Color / Grayscale"), COLOR_GRAYSCALE);
        colorModeSelector->addItem(tr("Mixed"), MIXED);

        thresholdMethodBox->addItem(tr("Otsu"), OTSU);
        thresholdMethodBox->addItem(tr("Sauvola"), SAUVOLA);
        thresholdMethodBox->addItem(tr("Wolf"), WOLF);

        fillingColorBox->addItem(tr("Background"), ColorCommonOptions::FillingColor::BACKGROUND);
        fillingColorBox->addItem(tr("White"), ColorCommonOptions::FillingColor::WHITE);

        QPointer<BinarizationOptionsWidget> otsuBinarizationOptionsWidget
                = new OtsuBinarizationOptionsWidget(m_ptrSettings);
        QPointer<BinarizationOptionsWidget> sauvolaBinarizationOptionsWidget
                = new SauvolaBinarizationOptionsWidget(m_ptrSettings);
        QPointer<BinarizationOptionsWidget> wolfBinarizationOptionsWidget
                = new WolfBinarizationOptionsWidget(m_ptrSettings);

        while (binarizationOptions->count() != 0) {
            binarizationOptions->removeWidget(binarizationOptions->widget(0));
        }
        addBinarizationOptionsWidget(otsuBinarizationOptionsWidget);
        addBinarizationOptionsWidget(sauvolaBinarizationOptionsWidget);
        addBinarizationOptionsWidget(wolfBinarizationOptionsWidget);
        updateBinarizationOptionsDisplay(binarizationOptions->currentIndex());

        pictureShapeSelector->addItem(tr("Free"), FREE_SHAPE);
        pictureShapeSelector->addItem(tr("Rectangular"), RECTANGULAR_SHAPE);

        updateDpiDisplay();
        updateColorsDisplay();
        updateDewarpingDisplay();

        connect(
                binarizationOptions, SIGNAL(currentChanged(int)),
                this, SLOT(updateBinarizationOptionsDisplay(int))
        );

        setupUiConnections();
    }

    OptionsWidget::~OptionsWidget() = default;

    void OptionsWidget::preUpdateUI(PageId const& page_id) {
        removeUiConnections();

        Params const params(m_ptrSettings->getParams(page_id));
        m_pageId = page_id;
        m_outputDpi = params.outputDpi();
        m_colorParams = params.colorParams();
        m_splittingOptions = params.splittingOptions();
        m_pictureShapeOptions = params.pictureShapeOptions();
        m_dewarpingOptions = params.dewarpingOptions();
        m_depthPerception = params.depthPerception();
        m_despeckleLevel = params.despeckleLevel();

        updateDpiDisplay();
        updateColorsDisplay();
        updateDewarpingDisplay();

        setupUiConnections();
    }

    void OptionsWidget::postUpdateUI() {
        removeUiConnections();

        m_colorParams = m_ptrSettings->getParams(m_pageId).colorParams();
        updateColorsDisplay();

        setupUiConnections();
    }

    void OptionsWidget::tabChanged(ImageViewTab const tab) {
        m_lastTab = tab;
        updateDpiDisplay();
        updateColorsDisplay();
        updateDewarpingDisplay();
        reloadIfNecessary();
    }

    void OptionsWidget::distortionModelChanged(dewarping::DistortionModel const& model) {
        m_ptrSettings->setDistortionModel(m_pageId, model);

        /*if (m_dewarpingOptions.mode() == AUTO)*/ {
            m_dewarpingOptions.setDewarpingMode(MANUAL);
            m_ptrSettings->setDewarpingOptions(m_pageId, m_dewarpingOptions);
            updateDewarpingDisplay();
        }
    }

    void OptionsWidget::colorModeChanged(int const idx) {
        int const mode = colorModeSelector->itemData(idx).toInt();
        m_colorParams.setColorMode((ColorMode) mode);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        updateColorsDisplay();
        emit reloadRequested();
    }

    void OptionsWidget::thresholdMethodChanged(int idx) {
        const BinarizationMethod method
                = (BinarizationMethod) thresholdMethodBox->itemData(idx).toInt();
        BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());
        blackWhiteOptions.setBinarizationMethod(method);
        m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        emit reloadRequested();
    }

    void OptionsWidget::fillingColorChanged(int idx) {
        const ColorCommonOptions::FillingColor color
                = (ColorCommonOptions::FillingColor) fillingColorBox->itemData(idx).toInt();
        ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
        colorCommonOptions.setFillingColor(color);
        m_colorParams.setColorCommonOptions(colorCommonOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        emit reloadRequested();
    }

    void OptionsWidget::pictureShapeChanged(int const idx) {
        m_pictureShapeOptions.setPictureShape((PictureShape) (pictureShapeSelector->itemData(idx).toInt()));
        m_ptrSettings->setPictureShapeOptions(m_pageId, m_pictureShapeOptions);

        emit reloadRequested();
    }

    void OptionsWidget::pictureShapeSensitivityChanged(int value) {
        m_pictureShapeOptions.setSensitivity(value);
        m_ptrSettings->setPictureShapeOptions(m_pageId, m_pictureShapeOptions);

        delayedReloadRequest.start(750);
    }

    void OptionsWidget::cutMarginsToggled(bool const checked) {
        ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
        colorCommonOptions.setCutMargins(checked);
        m_colorParams.setColorCommonOptions(colorCommonOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::equalizeIlluminationToggled(bool const checked) {
        BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());
        blackWhiteOptions.setNormalizeIllumination(checked);

        if (m_colorParams.colorMode() == MIXED) {
            if (!checked) {
                ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
                colorCommonOptions.setNormalizeIllumination(false);
                equalizeIlluminationColorCB->setChecked(false);
                m_colorParams.setColorCommonOptions(colorCommonOptions);
            }
            equalizeIlluminationColorCB->setEnabled(checked);
        }

        m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::equalizeIlluminationColorToggled(bool const checked) {
        ColorCommonOptions opt(m_colorParams.colorCommonOptions());
        opt.setNormalizeIllumination(checked);
        m_colorParams.setColorCommonOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }


    void OptionsWidget::binarizationSettingsChanged() {
        emit reloadRequested();
        emit invalidateThumbnail(m_pageId);
    }

    void OptionsWidget::changeDpiButtonClicked() {
        auto* dialog = new ChangeDpiDialog(
                this, m_outputDpi, m_pageId, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(
                dialog, SIGNAL(accepted(std::set<PageId> const &, Dpi const &)),
                this, SLOT(dpiChanged(std::set<PageId> const &, Dpi const &))
        );
        dialog->show();
    }

    void OptionsWidget::applyColorsButtonClicked() {
        auto* dialog = new ApplyColorsDialog(
                this, m_pageId, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(
                dialog, SIGNAL(accepted(std::set<PageId> const &)),
                this, SLOT(applyColorsConfirmed(std::set<PageId> const &))
        );
        dialog->show();
    }

    void OptionsWidget::dpiChanged(std::set<PageId> const& pages, Dpi const& dpi) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setDpi(page_id, dpi);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        if (pages.find(m_pageId) != pages.end()) {
            m_outputDpi = dpi;
            updateDpiDisplay();
            emit reloadRequested();
        }
    }

    void OptionsWidget::applyColorsConfirmed(std::set<PageId> const& pages) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setColorParams(page_id, m_colorParams);
            m_ptrSettings->setPictureShapeOptions(page_id, m_pictureShapeOptions);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        if (pages.find(m_pageId) != pages.end()) {
            emit reloadRequested();
        }
    }

    void OptionsWidget::applySplittingButtonClicked() {
        auto* dialog = new ApplyColorsDialog(
                this, m_pageId, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle(tr("Apply Splitting Settings"));
        connect(
                dialog, SIGNAL(accepted(std::set<PageId> const &)),
                this, SLOT(applySplittingOptionsConfirmed(std::set<PageId> const &))
        );
        dialog->show();
    }

    void OptionsWidget::applySplittingOptionsConfirmed(std::set<PageId> const& pages) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setSplittingOptions(page_id, m_splittingOptions);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        if (pages.find(m_pageId) != pages.end()) {
            emit reloadRequested();
        }
    }

    void OptionsWidget::despeckleOffSelected() {
        handleDespeckleLevelChange(DESPECKLE_OFF);
    }

    void OptionsWidget::despeckleCautiousSelected() {
        handleDespeckleLevelChange(DESPECKLE_CAUTIOUS);
    }

    void OptionsWidget::despeckleNormalSelected() {
        handleDespeckleLevelChange(DESPECKLE_NORMAL);
    }

    void OptionsWidget::despeckleAggressiveSelected() {
        handleDespeckleLevelChange(DESPECKLE_AGGRESSIVE);
    }

    void OptionsWidget::handleDespeckleLevelChange(DespeckleLevel const level) {
        m_despeckleLevel = level;
        m_ptrSettings->setDespeckleLevel(m_pageId, level);

        bool handled = false;
        emit despeckleLevelChanged(level, &handled);

        if (handled) {
            // This means we are on the "Despeckling" tab.
            emit invalidateThumbnail(m_pageId);
        } else {
            emit reloadRequested();
        }
    }

    void OptionsWidget::applyDespeckleButtonClicked() {
        auto* dialog = new ApplyColorsDialog(
                this, m_pageId, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle(tr("Apply Despeckling Level"));
        connect(
                dialog, SIGNAL(accepted(std::set<PageId> const &)),
                this, SLOT(applyDespeckleConfirmed(std::set<PageId> const &))
        );
        dialog->show();
    }

    void OptionsWidget::applyDespeckleConfirmed(std::set<PageId> const& pages) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setDespeckleLevel(page_id, m_despeckleLevel);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        if (pages.find(m_pageId) != pages.end()) {
            emit reloadRequested();
        }
    }

    void OptionsWidget::changeDewarpingButtonClicked() {
        auto* dialog = new ChangeDewarpingDialog(
                this, m_pageId, m_dewarpingOptions, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(
                dialog, SIGNAL(accepted(std::set<PageId> const &, DewarpingOptions const &)),
                this, SLOT(dewarpingChanged(std::set<PageId> const &, DewarpingOptions const &))
        );
        dialog->show();
    }

    void OptionsWidget::dewarpingChanged(std::set<PageId> const& pages, DewarpingOptions const& opt) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setDewarpingOptions(page_id, opt);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        if (pages.find(m_pageId) != pages.end()) {
            if (m_dewarpingOptions != opt) {
                m_dewarpingOptions = opt;


                // We also have to reload if we are currently on the "Fill Zones" tab,
                // as it makes use of original <-> dewarped coordinate mapping,
                // which is too hard to update without reloading.  For consistency,
                // we reload not just on TAB_FILL_ZONES but on all tabs except TAB_DEWARPING.
                // PS: the static original <-> dewarped mappings are constructed
                // in Task::UiUpdater::updateUI().  Look for "new DewarpingPointMapper" there.
                if ((opt.dewarpingMode() == AUTO)
                    || (m_lastTab != TAB_DEWARPING)
                    || (opt.dewarpingMode() == MARGINAL)) {
                    // Switch to the Output tab after reloading.
                    m_lastTab = TAB_OUTPUT;
                    // These depend on the value of m_lastTab.
                    updateDpiDisplay();
                    updateColorsDisplay();
                    updateDewarpingDisplay();

                    emit reloadRequested();
                } else {
                    // This one we have to call anyway, as it depends on m_dewarpingMode.
                    updateDewarpingDisplay();
                }
            }
        }
    }      // OptionsWidget::dewarpingChanged

    void OptionsWidget::applyDepthPerceptionButtonClicked() {
        auto* dialog = new ApplyColorsDialog(
                this, m_pageId, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle(tr("Apply Depth Perception"));
        connect(
                dialog, SIGNAL(accepted(std::set<PageId> const &)),
                this, SLOT(applyDepthPerceptionConfirmed(std::set<PageId> const &))
        );
        dialog->show();
    }

    void OptionsWidget::applyDepthPerceptionConfirmed(std::set<PageId> const& pages) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setDepthPerception(page_id, m_depthPerception);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        if (pages.find(m_pageId) != pages.end()) {
            emit reloadRequested();
        }
    }

    void OptionsWidget::depthPerceptionChangedSlot(int val) {
        m_depthPerception.setValue(0.1 * val);
        QString const tooltip_text(QString::number(m_depthPerception.value()));
        depthPerceptionSlider->setToolTip(tooltip_text);

        // Show the tooltip immediately.
        QPoint const center(depthPerceptionSlider->rect().center());
        QPoint tooltip_pos(depthPerceptionSlider->mapFromGlobal(QCursor::pos()));
        tooltip_pos.setY(center.y());
        tooltip_pos.setX(qBound(0, tooltip_pos.x(), depthPerceptionSlider->width()));
        tooltip_pos = depthPerceptionSlider->mapToGlobal(tooltip_pos);
        QToolTip::showText(tooltip_pos, tooltip_text, depthPerceptionSlider);

        // Propagate the signal.
        emit depthPerceptionChanged(m_depthPerception.value());
    }

    void OptionsWidget::reloadIfNecessary() {
        ZoneSet saved_picture_zones;
        ZoneSet saved_fill_zones;
        DewarpingOptions saved_dewarping_options;
        dewarping::DistortionModel saved_distortion_model;
        DepthPerception saved_depth_perception;
        DespeckleLevel saved_despeckle_level = DESPECKLE_CAUTIOUS;

        std::unique_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(m_pageId));
        if (output_params) {
            saved_picture_zones = output_params->pictureZones();
            saved_fill_zones = output_params->fillZones();
            saved_dewarping_options = output_params->outputImageParams().dewarpingMode();
            saved_distortion_model = output_params->outputImageParams().distortionModel();
            saved_depth_perception = output_params->outputImageParams().depthPerception();
            saved_despeckle_level = output_params->outputImageParams().despeckleLevel();
        }

        if (!PictureZoneComparator::equal(saved_picture_zones, m_ptrSettings->pictureZonesForPage(m_pageId))) {
            emit reloadRequested();

            return;
        } else if (!FillZoneComparator::equal(saved_fill_zones, m_ptrSettings->fillZonesForPage(m_pageId))) {
            emit reloadRequested();

            return;
        }

        Params const params(m_ptrSettings->getParams(m_pageId));

        if (saved_despeckle_level != params.despeckleLevel()) {
            emit reloadRequested();

            return;
        }

        if ((saved_dewarping_options.dewarpingMode() == OFF)
            && (params.dewarpingOptions().dewarpingMode() == OFF)) {
        } else if (saved_depth_perception.value() != params.depthPerception().value()) {
            emit reloadRequested();

            return;
        } else if ((saved_dewarping_options.dewarpingMode() == AUTO)
                   && (params.dewarpingOptions().dewarpingMode() == AUTO)) {
        } else if ((saved_dewarping_options.dewarpingMode() == MARGINAL)
                   && (params.dewarpingOptions().dewarpingMode() == MARGINAL)) {
        } else if (!saved_distortion_model.matches(params.distortionModel())) {
            emit reloadRequested();

            return;
        } else if ((saved_dewarping_options.dewarpingMode() == OFF)
                   != (params.dewarpingOptions().dewarpingMode() == OFF)) {
            emit reloadRequested();

            return;
        }
    }  // OptionsWidget::reloadIfNecessary

    void OptionsWidget::updateDpiDisplay() {
        if (m_outputDpi.horizontal() != m_outputDpi.vertical()) {
            dpiLabel->setText(
                    QString::fromLatin1("%1 x %2")
                            .arg(m_outputDpi.horizontal()).arg(m_outputDpi.vertical())
            );
        } else {
            dpiLabel->setText(QString::number(m_outputDpi.horizontal()));
        }
    }

    void OptionsWidget::updateColorsDisplay() {
        colorModeSelector->blockSignals(true);

        ColorMode const color_mode = m_colorParams.colorMode();
        int const color_mode_idx = colorModeSelector->findData(color_mode);
        colorModeSelector->setCurrentIndex(color_mode_idx);

        bool threshold_options_visible = false;
        bool picture_shape_visible = false;
        bool splitting_options_visible = false;
        switch (color_mode) {
            case MIXED:
                picture_shape_visible = true;
                splitting_options_visible = true;
                // fall into
            case BLACK_AND_WHITE:
                threshold_options_visible = true;
                // fall into
            case COLOR_GRAYSCALE:
                break;
        }

        commonOptions->setVisible(true);
        ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
        BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());

        if (!blackWhiteOptions.normalizeIllumination()
            && color_mode == MIXED) {
            colorCommonOptions.setNormalizeIllumination(false);
        }
        m_colorParams.setColorCommonOptions(colorCommonOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        cutMarginsCB->setChecked(colorCommonOptions.cutMargins());
        cutMarginsCB->setVisible(true);
        equalizeIlluminationCB->setChecked(blackWhiteOptions.normalizeIllumination());
        equalizeIlluminationCB->setVisible(color_mode != COLOR_GRAYSCALE);
        equalizeIlluminationColorCB->setChecked(colorCommonOptions.normalizeIllumination());
        equalizeIlluminationColorCB->setVisible(color_mode != BLACK_AND_WHITE);
        equalizeIlluminationColorCB->setEnabled(color_mode == COLOR_GRAYSCALE
                                                || blackWhiteOptions.normalizeIllumination());
        savitzkyGolaySmoothingCB->setChecked(blackWhiteOptions.isSavitzkyGolaySmoothingEnabled());
        savitzkyGolaySmoothingCB->setVisible(threshold_options_visible);
        morphologicalSmoothingCB->setChecked(blackWhiteOptions.isMorphologicalSmoothingEnabled());
        morphologicalSmoothingCB->setVisible(threshold_options_visible);

        modePanel->setVisible(m_lastTab != TAB_DEWARPING);
        pictureShapeOptions->setVisible(picture_shape_visible);
        thresholdOptions->setVisible(threshold_options_visible);
        despecklePanel->setVisible(threshold_options_visible && m_lastTab != TAB_DEWARPING);

        splittingOptions->setVisible(splitting_options_visible);
        splittingCB->setChecked(m_splittingOptions.isSplitOutput());
        switch (m_splittingOptions.getSplittingMode()) {
            case BLACK_AND_WHITE_FOREGROUND:
                bwForegroundRB->setChecked(true);
                break;
            case COLOR_FOREGROUND:
                colorForegroundRB->setChecked(true);
                break;
        }
        originalBackgroundCB->setChecked(m_splittingOptions.isOriginalBackground());
        colorForegroundRB->setEnabled(m_splittingOptions.isSplitOutput());
        bwForegroundRB->setEnabled(m_splittingOptions.isSplitOutput());
        originalBackgroundCB->setEnabled(m_splittingOptions.isSplitOutput()
                                         && (m_splittingOptions.getSplittingMode()
                                             == BLACK_AND_WHITE_FOREGROUND));

        thresholdMethodBox->setCurrentIndex((int) blackWhiteOptions.getBinarizationMethod());
        binarizationOptions->setCurrentIndex((int) blackWhiteOptions.getBinarizationMethod());

        fillingColorBox->setCurrentIndex((int) colorCommonOptions.getFillingColor());

        if (picture_shape_visible) {
            int const picture_shape_idx = pictureShapeSelector->findData(m_pictureShapeOptions.getPictureShape());
            pictureShapeSelector->setCurrentIndex(picture_shape_idx);
            pictureShapeSensitivitySB->setValue(m_pictureShapeOptions.getSensitivity());
            pictureShapeSensitivityOptions->setVisible(m_pictureShapeOptions.getPictureShape() == RECTANGULAR_SHAPE);
        }

        if (threshold_options_visible) {
            switch (m_despeckleLevel) {
                case DESPECKLE_OFF:
                    despeckleOffBtn->setChecked(true);
                    break;
                case DESPECKLE_CAUTIOUS:
                    despeckleCautiousBtn->setChecked(true);
                    break;
                case DESPECKLE_NORMAL:
                    despeckleNormalBtn->setChecked(true);
                    break;
                case DESPECKLE_AGGRESSIVE:
                    despeckleAggressiveBtn->setChecked(true);
                    break;
            }

            for (int i = 0; i < binarizationOptions->count(); i++) {
                auto* widget =
                        dynamic_cast<BinarizationOptionsWidget*>(binarizationOptions->widget(i));
                widget->preUpdateUI(m_pageId);
            }
        }

        colorModeSelector->blockSignals(false);
    }  // OptionsWidget::updateColorsDisplay

    void OptionsWidget::updateDewarpingDisplay() {
        depthPerceptionPanel->setVisible(m_lastTab == TAB_DEWARPING);

        switch (m_dewarpingOptions.dewarpingMode()) {
            case OFF:
                dewarpingStatusLabel->setText(tr("Off"));
                break;
            case AUTO:
                dewarpingStatusLabel->setText(tr("Auto"));
                break;
            case MANUAL:
                dewarpingStatusLabel->setText(tr("Manual"));
                break;
            case MARGINAL:
                dewarpingStatusLabel->setText(tr("Marginal"));
                break;
        }
        if (!m_dewarpingOptions.needPostDeskew()
            && ((m_dewarpingOptions.dewarpingMode() == MANUAL)
                || (m_dewarpingOptions.dewarpingMode() == MARGINAL))) {
            dewarpingStatusLabel->setText(dewarpingStatusLabel->text()
                                                  .append(" (").append(tr("deskew disabled"))
                                                  .append(")"));
        }

        depthPerceptionSlider->blockSignals(true);
        depthPerceptionSlider->setValue(qRound(m_depthPerception.value() * 10));
        depthPerceptionSlider->blockSignals(false);
    }

    void OptionsWidget::savitzkyGolaySmoothingToggled(bool checked) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setSavitzkyGolaySmoothingEnabled(checked);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::morphologicalSmoothingToggled(bool checked) {
        BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
        opt.setMorphologicalSmoothingEnabled(checked);
        m_colorParams.setBlackWhiteOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::bwForegroundToggled(bool checked) {
        if (!checked) {
            return;
        }

        originalBackgroundCB->setEnabled(checked);

        m_splittingOptions.setSplittingMode(BLACK_AND_WHITE_FOREGROUND);
        m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
        emit reloadRequested();
    }

    void OptionsWidget::colorForegroundToggled(bool checked) {
        if (!checked) {
            return;
        }

        originalBackgroundCB->setEnabled(!checked);

        m_splittingOptions.setSplittingMode(COLOR_FOREGROUND);
        m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
        emit reloadRequested();
    }

    void OptionsWidget::splittingToggled(bool checked) {
        m_splittingOptions.setSplitOutput(checked);

        bwForegroundRB->setEnabled(checked);
        colorForegroundRB->setEnabled(checked);
        originalBackgroundCB->setEnabled(checked && bwForegroundRB->isChecked());

        m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
        emit reloadRequested();
    }

    void OptionsWidget::originalBackgroundToggled(bool checked) {
        m_splittingOptions.setOriginalBackground(checked);

        m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
        emit reloadRequested();
    }

    void OptionsWidget::updateBinarizationOptionsDisplay(int idx) {
        for (int i = 0; i < binarizationOptions->count(); i++) {
            QWidget* currentWidget = binarizationOptions->widget(i);
            currentWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            currentWidget->resize(0, 0);

            disconnect(
                    currentWidget, SIGNAL(stateChanged()),
                    this, SLOT(binarizationSettingsChanged())
            );
        }

        QWidget* widget = binarizationOptions->widget(idx);
        widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        widget->adjustSize();
        binarizationOptions->adjustSize();

        connect(
                widget, SIGNAL(stateChanged()),
                this, SLOT(binarizationSettingsChanged())
        );
    }

    void OptionsWidget::addBinarizationOptionsWidget(BinarizationOptionsWidget* widget) {
        widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        binarizationOptions->addWidget(widget);
    }

    void OptionsWidget::sendReloadRequested() {
        emit reloadRequested();
    }

    void OptionsWidget::setupUiConnections() {
        connect(
                changeDpiButton, SIGNAL(clicked()),
                this, SLOT(changeDpiButtonClicked())
        );
        connect(
                colorModeSelector, SIGNAL(currentIndexChanged(int)),
                this, SLOT(colorModeChanged(int))
        );
        connect(
                thresholdMethodBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(thresholdMethodChanged(int))
        );
        connect(
                fillingColorBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(fillingColorChanged(int))
        );
        connect(
                pictureShapeSelector, SIGNAL(currentIndexChanged(int)),
                this, SLOT(pictureShapeChanged(int))
        );
        connect(
                pictureShapeSensitivitySB, SIGNAL(valueChanged(int)),
                this, SLOT(pictureShapeSensitivityChanged(int))
        );
        connect(
                cutMarginsCB, SIGNAL(clicked(bool)),
                this, SLOT(cutMarginsToggled(bool))
        );
        connect(
                equalizeIlluminationCB, SIGNAL(clicked(bool)),
                this, SLOT(equalizeIlluminationToggled(bool))
        );
        connect(
                equalizeIlluminationColorCB, SIGNAL(clicked(bool)),
                this, SLOT(equalizeIlluminationColorToggled(bool))
        );
        connect(
                savitzkyGolaySmoothingCB, SIGNAL(clicked(bool)),
                this, SLOT(savitzkyGolaySmoothingToggled(bool))
        );
        connect(
                morphologicalSmoothingCB, SIGNAL(clicked(bool)),
                this, SLOT(morphologicalSmoothingToggled(bool))
        );
        connect(
                splittingCB, SIGNAL(clicked(bool)),
                this, SLOT(splittingToggled(bool))
        );
        connect(
                bwForegroundRB, SIGNAL(clicked(bool)),
                this, SLOT(bwForegroundToggled(bool))
        );
        connect(
                colorForegroundRB, SIGNAL(clicked(bool)),
                this, SLOT(colorForegroundToggled(bool))
        );
        connect(
                originalBackgroundCB, SIGNAL(clicked(bool)),
                this, SLOT(originalBackgroundToggled(bool))
        );
        connect(
                applyColorsButton, SIGNAL(clicked()),
                this, SLOT(applyColorsButtonClicked())
        );

        connect(
                applySplittingButton, SIGNAL(clicked()),
                this, SLOT(applySplittingButtonClicked())
        );

        connect(
                changeDewarpingButton, SIGNAL(clicked()),
                this, SLOT(changeDewarpingButtonClicked())
        );

        connect(
                applyDepthPerceptionButton, SIGNAL(clicked()),
                this, SLOT(applyDepthPerceptionButtonClicked())
        );

        connect(
                despeckleOffBtn, SIGNAL(clicked()),
                this, SLOT(despeckleOffSelected())
        );
        connect(
                despeckleCautiousBtn, SIGNAL(clicked()),
                this, SLOT(despeckleCautiousSelected())
        );
        connect(
                despeckleNormalBtn, SIGNAL(clicked()),
                this, SLOT(despeckleNormalSelected())
        );
        connect(
                despeckleAggressiveBtn, SIGNAL(clicked()),
                this, SLOT(despeckleAggressiveSelected())
        );
        connect(
                applyDespeckleButton, SIGNAL(clicked()),
                this, SLOT(applyDespeckleButtonClicked())
        );
        connect(
                depthPerceptionSlider, SIGNAL(valueChanged(int)),
                this, SLOT(depthPerceptionChangedSlot(int))
        );
        connect(
                &delayedReloadRequest, SIGNAL(timeout()),
                this, SLOT(sendReloadRequested())
        );
    }

    void OptionsWidget::removeUiConnections() {
        disconnect(
                changeDpiButton, SIGNAL(clicked()),
                this, SLOT(changeDpiButtonClicked())
        );
        disconnect(
                colorModeSelector, SIGNAL(currentIndexChanged(int)),
                this, SLOT(colorModeChanged(int))
        );
        disconnect(
                thresholdMethodBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(thresholdMethodChanged(int))
        );
        disconnect(
                fillingColorBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(fillingColorChanged(int))
        );
        disconnect(
                pictureShapeSelector, SIGNAL(currentIndexChanged(int)),
                this, SLOT(pictureShapeChanged(int))
        );
        disconnect(
                pictureShapeSensitivitySB, SIGNAL(valueChanged(int)),
                this, SLOT(pictureShapeSensitivityChanged(int))
        );
        disconnect(
                cutMarginsCB, SIGNAL(clicked(bool)),
                this, SLOT(cutMarginsToggled(bool))
        );
        disconnect(
                equalizeIlluminationCB, SIGNAL(clicked(bool)),
                this, SLOT(equalizeIlluminationToggled(bool))
        );
        disconnect(
                equalizeIlluminationColorCB, SIGNAL(clicked(bool)),
                this, SLOT(equalizeIlluminationColorToggled(bool))
        );
        disconnect(
                savitzkyGolaySmoothingCB, SIGNAL(clicked(bool)),
                this, SLOT(savitzkyGolaySmoothingToggled(bool))
        );
        disconnect(
                morphologicalSmoothingCB, SIGNAL(clicked(bool)),
                this, SLOT(morphologicalSmoothingToggled(bool))
        );
        disconnect(
                splittingCB, SIGNAL(clicked(bool)),
                this, SLOT(splittingToggled(bool))
        );
        disconnect(
                bwForegroundRB, SIGNAL(clicked(bool)),
                this, SLOT(bwForegroundToggled(bool))
        );
        disconnect(
                colorForegroundRB, SIGNAL(clicked(bool)),
                this, SLOT(colorForegroundToggled(bool))
        );
        disconnect(
                originalBackgroundCB, SIGNAL(clicked(bool)),
                this, SLOT(originalBackgroundToggled(bool))
        );
        disconnect(
                applyColorsButton, SIGNAL(clicked()),
                this, SLOT(applyColorsButtonClicked())
        );

        disconnect(
                applySplittingButton, SIGNAL(clicked()),
                this, SLOT(applySplittingButtonClicked())
        );

        disconnect(
                changeDewarpingButton, SIGNAL(clicked()),
                this, SLOT(changeDewarpingButtonClicked())
        );

        disconnect(
                applyDepthPerceptionButton, SIGNAL(clicked()),
                this, SLOT(applyDepthPerceptionButtonClicked())
        );

        disconnect(
                despeckleOffBtn, SIGNAL(clicked()),
                this, SLOT(despeckleOffSelected())
        );
        disconnect(
                despeckleCautiousBtn, SIGNAL(clicked()),
                this, SLOT(despeckleCautiousSelected())
        );
        disconnect(
                despeckleNormalBtn, SIGNAL(clicked()),
                this, SLOT(despeckleNormalSelected())
        );
        disconnect(
                despeckleAggressiveBtn, SIGNAL(clicked()),
                this, SLOT(despeckleAggressiveSelected())
        );
        disconnect(
                applyDespeckleButton, SIGNAL(clicked()),
                this, SLOT(applyDespeckleButtonClicked())
        );
        disconnect(
                depthPerceptionSlider, SIGNAL(valueChanged(int)),
                this, SLOT(depthPerceptionChangedSlot(int))
        );
        disconnect(
                &delayedReloadRequest, SIGNAL(timeout()),
                this, SLOT(sendReloadRequested())
        );
    }

    ImageViewTab OptionsWidget::lastTab() const {
        return m_lastTab;
    }

    DepthPerception const& OptionsWidget::depthPerception() const {
        return m_depthPerception;
    }

}  // namespace output