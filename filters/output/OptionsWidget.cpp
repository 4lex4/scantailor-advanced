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
#include <tiff.h>

namespace output {
    OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings,
                                 PageSelectionAccessor const& page_selection_accessor)
            : m_ptrSettings(settings),
              m_pageSelectionAccessor(page_selection_accessor),
              m_despeckleLevel(DESPECKLE_NORMAL),
              m_lastTab(TAB_OUTPUT) {
        setupUi(this);

        depthPerceptionSlider->setMinimum(qRound(DepthPerception::minValue() * 10));
        depthPerceptionSlider->setMaximum(qRound(DepthPerception::maxValue() * 10));

        colorModeSelector->addItem(tr("Black and White"), ColorParams::BLACK_AND_WHITE);
        colorModeSelector->addItem(tr("Color / Grayscale"), ColorParams::COLOR_GRAYSCALE);
        colorModeSelector->addItem(tr("Mixed"), ColorParams::MIXED);

        thresholdMethodBox->addItem(tr("Otsu"), BlackWhiteOptions::OTSU);
        thresholdMethodBox->addItem(tr("Sauvola"), BlackWhiteOptions::SAUVOLA);
        thresholdMethodBox->addItem(tr("Wolf"), BlackWhiteOptions::WOLF);

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
        binarizationOptionsChanged(binarizationOptions->currentIndex());

        pictureShapeSelector->addItem(tr("Free"), FREE_SHAPE);
        pictureShapeSelector->addItem(tr("Rectangular"), RECTANGULAR_SHAPE);
        pictureShapeSelector->addItem(tr("Quadro"), QUADRO_SHAPE);

        tiffCompression->addItem(tr("None"), COMPRESSION_NONE);
        tiffCompression->addItem(tr("LZW"), COMPRESSION_LZW);
        tiffCompression->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
        tiffCompression->addItem(tr("Packbits"), COMPRESSION_PACKBITS);
        tiffCompression->addItem(tr("JPEG"), COMPRESSION_JPEG);

        updateDpiDisplay();
        updateColorsDisplay();
        updateDewarpingDisplay();

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
                binarizationOptions, SIGNAL(currentChanged(int)),
                this, SLOT(binarizationOptionsChanged(int))
        );
        connect(
                pictureShapeSelector, SIGNAL(currentIndexChanged(int)),
                this, SLOT(pictureShapeChanged(int))
        );
        connect(
                tiffCompression, SIGNAL(currentIndexChanged(int)),
                this, SLOT(tiffCompressionChanged(int))
        );
        connect(
                whiteMarginsCB, SIGNAL(clicked(bool)),
                this, SLOT(whiteMarginsToggled(bool))
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
                applyColorsButton, SIGNAL(clicked()),
                this, SLOT(applyColorsButtonClicked())
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


    }

    OptionsWidget::~OptionsWidget() {
    }

    void OptionsWidget::preUpdateUI(PageId const& page_id) {
        Params const params(m_ptrSettings->getParams(page_id));
        m_pageId = page_id;
        m_outputDpi = params.outputDpi();
        m_colorParams = params.colorParams();
        m_pictureShape = params.pictureShape();
        m_dewarpingMode = params.dewarpingMode();
        m_depthPerception = params.depthPerception();
        m_despeckleLevel = params.despeckleLevel();

        updateDpiDisplay();
        updateColorsDisplay();
        updateDewarpingDisplay();
        for (int i = 0; i < binarizationOptions->count(); i++) {
            BinarizationOptionsWidget* widget =
                    dynamic_cast<BinarizationOptionsWidget*>(binarizationOptions->widget(i));
            widget->preUpdateUI(m_pageId);
        }
    }

    void OptionsWidget::postUpdateUI() {
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

        /*if (m_dewarpingMode == DewarpingMode::AUTO)*/ {
            m_ptrSettings->setDewarpingMode(m_pageId, DewarpingMode::MANUAL);
            m_dewarpingMode = DewarpingMode::MANUAL;
            updateDewarpingDisplay();
        }
    }

    void OptionsWidget::colorModeChanged(int const idx) {
        int const mode = colorModeSelector->itemData(idx).toInt();
        m_colorParams.setColorMode((ColorParams::ColorMode) mode);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        updateColorsDisplay();
        emit reloadRequested();
    }

    void OptionsWidget::thresholdMethodChanged(int idx) {
        const BlackWhiteOptions::BinarizationMethod method
                = (BlackWhiteOptions::BinarizationMethod) thresholdMethodBox->itemData(idx).toInt();
        BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());
        blackWhiteOptions.setBinarizationMethod(method);
        m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        binarizationOptions->setCurrentIndex(idx);

        emit reloadRequested();
    }

    void OptionsWidget::pictureShapeChanged(int const idx) {
        m_pictureShape = (PictureShape) (pictureShapeSelector->itemData(idx).toInt());
        m_ptrSettings->setPictureShape(m_pageId, m_pictureShape);
        emit reloadRequested();
    }

    void OptionsWidget::tiffCompressionChanged(int idx) {
        int compression = tiffCompression->itemData(idx).toInt();
        m_ptrSettings->setTiffCompression(compression);
    }

    void OptionsWidget::whiteMarginsToggled(bool const checked) {
        ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
        colorCommonOptions.setWhiteMargins(checked);

        if (!checked) {
            BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());
            blackWhiteOptions.setNormalizeIllumination(false);
            equalizeIlluminationCB->setChecked(false);
            m_colorParams.setBlackWhiteOptions(blackWhiteOptions);

            colorCommonOptions.setNormalizeIllumination(false);
            equalizeIlluminationColorCB->setChecked(false);
        }
        equalizeIlluminationCB->setEnabled(checked);
        equalizeIlluminationColorCB->setEnabled(checked);

        m_colorParams.setColorCommonOptions(colorCommonOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::equalizeIlluminationToggled(bool const checked) {
        BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());
        blackWhiteOptions.setNormalizeIllumination(checked);

        if (m_colorParams.colorMode() == ColorParams::MIXED) {
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
        ChangeDpiDialog* dialog = new ChangeDpiDialog(
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
        ApplyColorsDialog* dialog = new ApplyColorsDialog(
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
            m_ptrSettings->setPictureShape(page_id, m_pictureShape);
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
            emit invalidateThumbnail(m_pageId);
        } else {
            emit reloadRequested();
        }
    }

    void OptionsWidget::applyDespeckleButtonClicked() {
        ApplyColorsDialog* dialog = new ApplyColorsDialog(
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
        ChangeDewarpingDialog* dialog = new ChangeDewarpingDialog(
                this, m_pageId, m_dewarpingMode, m_pageSelectionAccessor
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(
                dialog, SIGNAL(accepted(std::set<PageId> const &, DewarpingMode const &)),
                this, SLOT(dewarpingChanged(std::set<PageId> const &, DewarpingMode const &))
        );
        dialog->show();
    }

    void OptionsWidget::dewarpingChanged(std::set<PageId> const& pages, DewarpingMode const& mode) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setDewarpingMode(page_id, mode);
        }

        if (pages.size() > 1) {
            emit invalidateAllThumbnails();
        } else {
            for (PageId const& page_id : pages) {
                emit invalidateThumbnail(page_id);
            }
        }

        if (pages.find(m_pageId) != pages.end()) {
            if (m_dewarpingMode != mode) {
                m_dewarpingMode = mode;

                if ((mode == DewarpingMode::AUTO) || (m_lastTab != TAB_DEWARPING)
                    || (mode == DewarpingMode::MARGINAL)
                        ) {
                    m_lastTab = TAB_OUTPUT;

                    updateDpiDisplay();
                    updateColorsDisplay();
                    updateDewarpingDisplay();

                    emit reloadRequested();
                } else {
                    updateDewarpingDisplay();
                }
            }
        }
    }      // OptionsWidget::dewarpingChanged

    void OptionsWidget::applyDepthPerceptionButtonClicked() {
        ApplyColorsDialog* dialog = new ApplyColorsDialog(
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

        QPoint const center(depthPerceptionSlider->rect().center());
        QPoint tooltip_pos(depthPerceptionSlider->mapFromGlobal(QCursor::pos()));
        tooltip_pos.setY(center.y());
        tooltip_pos.setX(qBound(0, tooltip_pos.x(), depthPerceptionSlider->width()));
        tooltip_pos = depthPerceptionSlider->mapToGlobal(tooltip_pos);
        QToolTip::showText(tooltip_pos, tooltip_text, depthPerceptionSlider);

        emit depthPerceptionChanged(m_depthPerception.value());
    }

    void OptionsWidget::reloadIfNecessary() {
        ZoneSet saved_picture_zones;
        ZoneSet saved_fill_zones;
        DewarpingMode saved_dewarping_mode;
        dewarping::DistortionModel saved_distortion_model;
        DepthPerception saved_depth_perception;
        DespeckleLevel saved_despeckle_level = DESPECKLE_CAUTIOUS;

        std::unique_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(m_pageId));
        if (output_params) {
            saved_picture_zones = output_params->pictureZones();
            saved_fill_zones = output_params->fillZones();
            saved_dewarping_mode = output_params->outputImageParams().dewarpingMode();
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

        if ((saved_dewarping_mode == DewarpingMode::OFF) && (params.dewarpingMode() == DewarpingMode::OFF)) {
        } else if (saved_depth_perception.value() != params.depthPerception().value()) {
            emit reloadRequested();

            return;
        } else if ((saved_dewarping_mode == DewarpingMode::AUTO) && (params.dewarpingMode() == DewarpingMode::AUTO)) {
        } else if ((saved_dewarping_mode == DewarpingMode::MARGINAL)
                   && (params.dewarpingMode() == DewarpingMode::MARGINAL)) {
        } else if (!saved_distortion_model.matches(params.distortionModel())) {
            emit reloadRequested();

            return;
        } else if ((saved_dewarping_mode == DewarpingMode::OFF) != (params.dewarpingMode() == DewarpingMode::OFF)) {
            emit reloadRequested();

            return;
        }
    }      // OptionsWidget::reloadIfNecessary

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

        ColorParams::ColorMode const color_mode = m_colorParams.colorMode();
        int const color_mode_idx = colorModeSelector->findData(color_mode);
        colorModeSelector->setCurrentIndex(color_mode_idx);

        bool threshold_options_visible = false;
        bool picture_shape_visible = false;
        bool splitting_options_visible = false;
        switch (color_mode) {
            case ColorParams::BLACK_AND_WHITE:
                threshold_options_visible = true;
                break;
            case ColorParams::COLOR_GRAYSCALE:
                break;
            case ColorParams::MIXED:
                threshold_options_visible = true;
                picture_shape_visible = true;
                splitting_options_visible = true;
                break;
        }

        commonOptions->setVisible(true);
        ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
        BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());
        SplittingOptions splitOptions(m_colorParams.splittingOptions());

        if (!colorCommonOptions.whiteMargins()) {
            colorCommonOptions.setNormalizeIllumination(false);
            blackWhiteOptions.setNormalizeIllumination(false);
        } else if (!blackWhiteOptions.normalizeIllumination()
                   && color_mode == ColorParams::MIXED) {
            colorCommonOptions.setNormalizeIllumination(false);
        }
        m_colorParams.setColorCommonOptions(colorCommonOptions);
        m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);

        whiteMarginsCB->setChecked(colorCommonOptions.whiteMargins());
        whiteMarginsCB->setVisible(true);
        equalizeIlluminationCB->setChecked(blackWhiteOptions.normalizeIllumination());
        equalizeIlluminationCB->setEnabled(colorCommonOptions.whiteMargins());
        equalizeIlluminationCB->setVisible(color_mode != ColorParams::COLOR_GRAYSCALE);
        equalizeIlluminationColorCB->setChecked(colorCommonOptions.normalizeIllumination());
        equalizeIlluminationColorCB->setVisible(color_mode != ColorParams::BLACK_AND_WHITE);
        if (color_mode != ColorParams::MIXED) {
            equalizeIlluminationColorCB->setEnabled(colorCommonOptions.whiteMargins());
        } else {
            equalizeIlluminationColorCB->setEnabled(colorCommonOptions.whiteMargins()
                                                    && blackWhiteOptions.normalizeIllumination());
        }
        savitzkyGolaySmoothingCB->setChecked(blackWhiteOptions.isSavitzkyGolaySmoothingEnabled());
        savitzkyGolaySmoothingCB->setVisible(threshold_options_visible);
        morphologicalSmoothingCB->setChecked(blackWhiteOptions.isMorphologicalSmoothingEnabled());
        morphologicalSmoothingCB->setVisible(threshold_options_visible);

        modePanel->setVisible(m_lastTab != TAB_DEWARPING);
        pictureShapeOptions->setVisible(picture_shape_visible);
        thresholdOptions->setVisible(threshold_options_visible);
        despecklePanel->setVisible(threshold_options_visible && m_lastTab != TAB_DEWARPING);

        splittingOptions->setVisible(splitting_options_visible);
        splittingCB->setChecked(splitOptions.isSplitOutput());
        switch (splitOptions.getForegroundType()) {
            case SplittingOptions::BLACK_AND_WHITE_FOREGROUND:
                bwForegroundRB->setChecked(true);
                break;
            case SplittingOptions::COLOR_FOREGROUND:
                colorForegroundRB->setChecked(true);
                break;
        }
        colorForegroundRB->setEnabled(splitOptions.isSplitOutput());
        bwForegroundRB->setEnabled(splitOptions.isSplitOutput());

        thresholdMethodBox->setCurrentIndex((int) blackWhiteOptions.getBinarizationMethod());
        binarizationOptions->setCurrentIndex((int) blackWhiteOptions.getBinarizationMethod());

        if (picture_shape_visible) {
            int const picture_shape_idx = pictureShapeSelector->findData(m_pictureShape);
            pictureShapeSelector->setCurrentIndex(picture_shape_idx);
        }

        int compression_idx = tiffCompression->findData(m_ptrSettings->getTiffCompression());
        tiffCompression->setCurrentIndex(compression_idx);

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
        }

        colorModeSelector->blockSignals(false);
    }      // OptionsWidget::updateColorsDisplay

    void OptionsWidget::updateDewarpingDisplay() {
        depthPerceptionPanel->setVisible(m_lastTab == TAB_DEWARPING);

        switch (m_dewarpingMode) {
            case DewarpingMode::OFF:
                dewarpingStatusLabel->setText(tr("Off"));
                break;
            case DewarpingMode::AUTO:
                dewarpingStatusLabel->setText(tr("Auto"));
                break;
            case DewarpingMode::MANUAL:
                dewarpingStatusLabel->setText(tr("Manual"));
                break;
            case DewarpingMode::MARGINAL:
                dewarpingStatusLabel->setText(tr("Marginal"));
                break;
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
        SplittingOptions opt(m_colorParams.splittingOptions());
        opt.setForegroundType(SplittingOptions::BLACK_AND_WHITE_FOREGROUND);
        m_colorParams.setSplittingOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::colorForegroundToggled(bool checked) {
        if (!checked) {
            return;
        }
        SplittingOptions opt(m_colorParams.splittingOptions());
        opt.setForegroundType(SplittingOptions::COLOR_FOREGROUND);
        m_colorParams.setSplittingOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::splittingToggled(bool checked) {
        SplittingOptions opt(m_colorParams.splittingOptions());
        opt.setSplitOutput(checked);

        bwForegroundRB->setEnabled(checked);
        colorForegroundRB->setEnabled(checked);

        m_colorParams.setSplittingOptions(opt);
        m_ptrSettings->setColorParams(m_pageId, m_colorParams);
        emit reloadRequested();
    }

    void OptionsWidget::binarizationOptionsChanged(int idx) {
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

}  // namespace output