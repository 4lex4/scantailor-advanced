// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"
#include <QToolTip>
#include <utility>
#include "../../Utils.h"
#include "ApplyColorsDialog.h"
#include "ChangeDewarpingDialog.h"
#include "ChangeDpiDialog.h"
#include "FillZoneComparator.h"
#include "OtsuBinarizationOptionsWidget.h"
#include "PictureZoneComparator.h"
#include "SauvolaBinarizationOptionsWidget.h"
#include "WolfBinarizationOptionsWidget.h"

using namespace core;

namespace output {
OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor)
    : m_settings(std::move(settings)),
      m_pageSelectionAccessor(page_selection_accessor),
      m_despeckleLevel(1.0),
      m_lastTab(TAB_OUTPUT) {
  setupUi(this);

  m_delayedReloadRequest.setSingleShot(true);

  depthPerceptionSlider->setMinimum(qRound(DepthPerception::minValue() * 10));
  depthPerceptionSlider->setMaximum(qRound(DepthPerception::maxValue() * 10));

  despeckleSlider->setMinimum(qRound(1.0 * 10));
  despeckleSlider->setMaximum(qRound(3.0 * 10));

  colorModeSelector->addItem(tr("Black and White"), BLACK_AND_WHITE);
  colorModeSelector->addItem(tr("Color / Grayscale"), COLOR_GRAYSCALE);
  colorModeSelector->addItem(tr("Mixed"), MIXED);

  thresholdMethodBox->addItem(tr("Otsu"), OTSU);
  thresholdMethodBox->addItem(tr("Sauvola"), SAUVOLA);
  thresholdMethodBox->addItem(tr("Wolf"), WOLF);

  fillingColorBox->addItem(tr("Background"), FILL_BACKGROUND);
  fillingColorBox->addItem(tr("White"), FILL_WHITE);

  QPointer<BinarizationOptionsWidget> otsuBinarizationOptionsWidget = new OtsuBinarizationOptionsWidget(m_settings);
  QPointer<BinarizationOptionsWidget> sauvolaBinarizationOptionsWidget
      = new SauvolaBinarizationOptionsWidget(m_settings);
  QPointer<BinarizationOptionsWidget> wolfBinarizationOptionsWidget = new WolfBinarizationOptionsWidget(m_settings);

  while (binarizationOptions->count() != 0) {
    binarizationOptions->removeWidget(binarizationOptions->widget(0));
  }
  addBinarizationOptionsWidget(otsuBinarizationOptionsWidget);
  addBinarizationOptionsWidget(sauvolaBinarizationOptionsWidget);
  addBinarizationOptionsWidget(wolfBinarizationOptionsWidget);
  updateBinarizationOptionsDisplay(binarizationOptions->currentIndex());

  pictureShapeSelector->addItem(tr("Off"), OFF_SHAPE);
  pictureShapeSelector->addItem(tr("Free"), FREE_SHAPE);
  pictureShapeSelector->addItem(tr("Rectangular"), RECTANGULAR_SHAPE);

  updateDpiDisplay();
  updateColorsDisplay();
  updateDewarpingDisplay();

  connect(binarizationOptions, SIGNAL(currentChanged(int)), this, SLOT(updateBinarizationOptionsDisplay(int)));

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageId& page_id) {
  removeUiConnections();

  const Params params = m_settings->getParams(page_id);
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
  updateProcessingDisplay();

  setupUiConnections();
}

void OptionsWidget::postUpdateUI() {
  removeUiConnections();

  updateProcessingDisplay();
  m_dewarpingOptions = m_settings->getParams(m_pageId).dewarpingOptions();
  updateDewarpingDisplay();

  setupUiConnections();
}

void OptionsWidget::tabChanged(const ImageViewTab tab) {
  m_lastTab = tab;
  updateDpiDisplay();
  updateColorsDisplay();
  updateDewarpingDisplay();
  reloadIfNecessary();
}

void OptionsWidget::distortionModelChanged(const dewarping::DistortionModel& model) {
  m_settings->setDistortionModel(m_pageId, model);

  m_dewarpingOptions.setDewarpingMode(MANUAL);
  m_settings->setDewarpingOptions(m_pageId, m_dewarpingOptions);
  updateDewarpingDisplay();
}

void OptionsWidget::colorModeChanged(const int idx) {
  const int mode = colorModeSelector->itemData(idx).toInt();
  m_colorParams.setColorMode((ColorMode) mode);
  m_settings->setColorParams(m_pageId, m_colorParams);
  updateColorsDisplay();
  emit reloadRequested();
}

void OptionsWidget::thresholdMethodChanged(int idx) {
  const BinarizationMethod method = (BinarizationMethod) thresholdMethodBox->itemData(idx).toInt();
  BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());
  blackWhiteOptions.setBinarizationMethod(method);
  m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  emit reloadRequested();
}

void OptionsWidget::fillingColorChanged(int idx) {
  const FillingColor color = (FillingColor) fillingColorBox->itemData(idx).toInt();
  ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
  colorCommonOptions.setFillingColor(color);
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  emit reloadRequested();
}

void OptionsWidget::pictureShapeChanged(const int idx) {
  const auto shapeMode = static_cast<PictureShape>(pictureShapeSelector->itemData(idx).toInt());
  m_pictureShapeOptions.setPictureShape(shapeMode);
  m_settings->setPictureShapeOptions(m_pageId, m_pictureShapeOptions);

  pictureShapeSensitivityOptions->setVisible(shapeMode == RECTANGULAR_SHAPE);
  higherSearchSensitivityCB->setVisible(shapeMode != OFF_SHAPE);

  emit reloadRequested();
}

void OptionsWidget::pictureShapeSensitivityChanged(int value) {
  m_pictureShapeOptions.setSensitivity(value);
  m_settings->setPictureShapeOptions(m_pageId, m_pictureShapeOptions);

  m_delayedReloadRequest.start(750);
}

void OptionsWidget::higherSearchSensivityToggled(const bool checked) {
  m_pictureShapeOptions.setHigherSearchSensitivity(checked);
  m_settings->setPictureShapeOptions(m_pageId, m_pictureShapeOptions);

  emit reloadRequested();
}

void OptionsWidget::fillMarginsToggled(const bool checked) {
  ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
  colorCommonOptions.setFillMargins(checked);
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);
  emit reloadRequested();
}

void OptionsWidget::fillOffcutToggled(const bool checked) {
  ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
  colorCommonOptions.setFillOffcut(checked);
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);
  emit reloadRequested();
}

void OptionsWidget::equalizeIlluminationToggled(const bool checked) {
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
  m_settings->setColorParams(m_pageId, m_colorParams);
  emit reloadRequested();
}

void OptionsWidget::equalizeIlluminationColorToggled(const bool checked) {
  ColorCommonOptions opt(m_colorParams.colorCommonOptions());
  opt.setNormalizeIllumination(checked);
  m_colorParams.setColorCommonOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);
  emit reloadRequested();
}


void OptionsWidget::binarizationSettingsChanged() {
  emit reloadRequested();
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::changeDpiButtonClicked() {
  auto* dialog = new ChangeDpiDialog(this, m_outputDpi, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&, const Dpi&)), this,
          SLOT(dpiChanged(const std::set<PageId>&, const Dpi&)));
  dialog->show();
}

void OptionsWidget::applyColorsButtonClicked() {
  auto* dialog = new ApplyColorsDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&)), this, SLOT(applyColorsConfirmed(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::dpiChanged(const std::set<PageId>& pages, const Dpi& dpi) {
  for (const PageId& page_id : pages) {
    m_settings->setDpi(page_id, dpi);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }

  if (pages.find(m_pageId) != pages.end()) {
    m_outputDpi = dpi;
    updateDpiDisplay();
    emit reloadRequested();
  }
}

void OptionsWidget::applyColorsConfirmed(const std::set<PageId>& pages) {
  for (const PageId& page_id : pages) {
    m_settings->setColorParams(page_id, m_colorParams);
    m_settings->setPictureShapeOptions(page_id, m_pictureShapeOptions);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }

  if (pages.find(m_pageId) != pages.end()) {
    emit reloadRequested();
  }
}

void OptionsWidget::applySplittingButtonClicked() {
  auto* dialog = new ApplyColorsDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Splitting Settings"));
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&)), this,
          SLOT(applySplittingOptionsConfirmed(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::applySplittingOptionsConfirmed(const std::set<PageId>& pages) {
  for (const PageId& page_id : pages) {
    m_settings->setSplittingOptions(page_id, m_splittingOptions);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }

  if (pages.find(m_pageId) != pages.end()) {
    emit reloadRequested();
  }
}

void OptionsWidget::despeckleToggled(bool checked) {
  if (checked) {
    handleDespeckleLevelChange(0.1 * despeckleSlider->value());
  } else {
    handleDespeckleLevelChange(0);
  };

  despeckleSlider->setEnabled(checked);
}

void OptionsWidget::despeckleSliderReleased() {
  const double value = 0.1 * despeckleSlider->value();
  handleDespeckleLevelChange(value);
}

void OptionsWidget::despeckleSliderValueChanged(int value) {
  const double new_value = 0.1 * value;

  const QString tooltip_text(QString::number(new_value));
  despeckleSlider->setToolTip(tooltip_text);

  // Show the tooltip immediately.
  const QPoint center(despeckleSlider->rect().center());
  QPoint tooltip_pos(despeckleSlider->mapFromGlobal(QCursor::pos()));
  tooltip_pos.setY(center.y());
  tooltip_pos.setX(qBound(0, tooltip_pos.x(), despeckleSlider->width()));
  tooltip_pos = despeckleSlider->mapToGlobal(tooltip_pos);
  QToolTip::showText(tooltip_pos, tooltip_text, despeckleSlider);

  if (despeckleSlider->isSliderDown()) {
    return;
  }

  handleDespeckleLevelChange(new_value, true);
}

void OptionsWidget::handleDespeckleLevelChange(const double level, const bool delay) {
  m_despeckleLevel = level;
  m_settings->setDespeckleLevel(m_pageId, level);

  bool handled = false;
  emit despeckleLevelChanged(level, &handled);

  if (handled) {
    // This means we are on the "Despeckling" tab.
    emit invalidateThumbnail(m_pageId);
  } else {
    if (delay) {
      m_delayedReloadRequest.start(750);
    } else {
      emit reloadRequested();
    }
  }
}

void OptionsWidget::applyDespeckleButtonClicked() {
  auto* dialog = new ApplyColorsDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Despeckling Level"));
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&)), this,
          SLOT(applyDespeckleConfirmed(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::applyDespeckleConfirmed(const std::set<PageId>& pages) {
  for (const PageId& page_id : pages) {
    m_settings->setDespeckleLevel(page_id, m_despeckleLevel);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }

  if (pages.find(m_pageId) != pages.end()) {
    emit reloadRequested();
  }
}

void OptionsWidget::changeDewarpingButtonClicked() {
  auto* dialog = new ChangeDewarpingDialog(this, m_pageId, m_dewarpingOptions, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&, const DewarpingOptions&)), this,
          SLOT(dewarpingChanged(const std::set<PageId>&, const DewarpingOptions&)));
  dialog->show();
}

void OptionsWidget::dewarpingChanged(const std::set<PageId>& pages, const DewarpingOptions& opt) {
  for (const PageId& page_id : pages) {
    m_settings->setDewarpingOptions(page_id, opt);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
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
      if ((opt.dewarpingMode() == AUTO) || (m_lastTab != TAB_DEWARPING) || (opt.dewarpingMode() == MARGINAL)) {
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
}  // OptionsWidget::dewarpingChanged

void OptionsWidget::applyDepthPerceptionButtonClicked() {
  auto* dialog = new ApplyColorsDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Depth Perception"));
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&)), this,
          SLOT(applyDepthPerceptionConfirmed(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::applyDepthPerceptionConfirmed(const std::set<PageId>& pages) {
  for (const PageId& page_id : pages) {
    m_settings->setDepthPerception(page_id, m_depthPerception);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }

  if (pages.find(m_pageId) != pages.end()) {
    emit reloadRequested();
  }
}

void OptionsWidget::depthPerceptionChangedSlot(int val) {
  m_depthPerception.setValue(0.1 * val);
  const QString tooltip_text(QString::number(m_depthPerception.value()));
  depthPerceptionSlider->setToolTip(tooltip_text);

  // Show the tooltip immediately.
  const QPoint center(depthPerceptionSlider->rect().center());
  QPoint tooltip_pos(depthPerceptionSlider->mapFromGlobal(QCursor::pos()));
  tooltip_pos.setY(center.y());
  tooltip_pos.setX(qBound(0, tooltip_pos.x(), depthPerceptionSlider->width()));
  tooltip_pos = depthPerceptionSlider->mapToGlobal(tooltip_pos);
  QToolTip::showText(tooltip_pos, tooltip_text, depthPerceptionSlider);

  m_settings->setDepthPerception(m_pageId, m_depthPerception);
  // Propagate the signal.
  emit depthPerceptionChanged(m_depthPerception.value());
}

void OptionsWidget::reloadIfNecessary() {
  ZoneSet saved_picture_zones;
  ZoneSet saved_fill_zones;
  DewarpingOptions saved_dewarping_options;
  dewarping::DistortionModel saved_distortion_model;
  DepthPerception saved_depth_perception;
  double saved_despeckle_level = 1.0;

  std::unique_ptr<OutputParams> output_params(m_settings->getOutputParams(m_pageId));
  if (output_params) {
    saved_picture_zones = output_params->pictureZones();
    saved_fill_zones = output_params->fillZones();
    saved_dewarping_options = output_params->outputImageParams().dewarpingMode();
    saved_distortion_model = output_params->outputImageParams().distortionModel();
    saved_depth_perception = output_params->outputImageParams().depthPerception();
    saved_despeckle_level = output_params->outputImageParams().despeckleLevel();
  }

  if (!PictureZoneComparator::equal(saved_picture_zones, m_settings->pictureZonesForPage(m_pageId))) {
    emit reloadRequested();

    return;
  } else if (!FillZoneComparator::equal(saved_fill_zones, m_settings->fillZonesForPage(m_pageId))) {
    emit reloadRequested();

    return;
  }

  const Params params(m_settings->getParams(m_pageId));

  if (saved_despeckle_level != params.despeckleLevel()) {
    emit reloadRequested();

    return;
  }

  if ((saved_dewarping_options.dewarpingMode() == OFF) && (params.dewarpingOptions().dewarpingMode() == OFF)) {
  } else if (saved_depth_perception.value() != params.depthPerception().value()) {
    emit reloadRequested();

    return;
  } else if ((saved_dewarping_options.dewarpingMode() == AUTO) && (params.dewarpingOptions().dewarpingMode() == AUTO)) {
  } else if ((saved_dewarping_options.dewarpingMode() == MARGINAL)
             && (params.dewarpingOptions().dewarpingMode() == MARGINAL)) {
  } else if (!saved_distortion_model.matches(params.distortionModel())) {
    emit reloadRequested();

    return;
  } else if ((saved_dewarping_options.dewarpingMode() == OFF) != (params.dewarpingOptions().dewarpingMode() == OFF)) {
    emit reloadRequested();

    return;
  }
}  // OptionsWidget::reloadIfNecessary

void OptionsWidget::updateDpiDisplay() {
  if (m_outputDpi.horizontal() != m_outputDpi.vertical()) {
    dpiLabel->setText(QString::fromLatin1("%1 x %2").arg(m_outputDpi.horizontal()).arg(m_outputDpi.vertical()));
  } else {
    dpiLabel->setText(QString::number(m_outputDpi.horizontal()));
  }
}

void OptionsWidget::updateColorsDisplay() {
  colorModeSelector->blockSignals(true);

  const ColorMode color_mode = m_colorParams.colorMode();
  const int color_mode_idx = colorModeSelector->findData(color_mode);
  colorModeSelector->setCurrentIndex(color_mode_idx);

  bool threshold_options_visible = false;
  bool picture_shape_visible = false;
  bool splitting_options_visible = false;
  switch (color_mode) {
    case MIXED:
      picture_shape_visible = true;
      splitting_options_visible = true;
      // fall through
    case BLACK_AND_WHITE:
      threshold_options_visible = true;
      // fall through
    case COLOR_GRAYSCALE:
      break;
  }

  commonOptions->setVisible(true);
  ColorCommonOptions colorCommonOptions(m_colorParams.colorCommonOptions());
  BlackWhiteOptions blackWhiteOptions(m_colorParams.blackWhiteOptions());

  if (!blackWhiteOptions.normalizeIllumination() && color_mode == MIXED) {
    colorCommonOptions.setNormalizeIllumination(false);
  }
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  fillMarginsCB->setChecked(colorCommonOptions.fillMargins());
  fillMarginsCB->setVisible(true);
  fillOffcutCB->setChecked(colorCommonOptions.fillOffcut());
  fillOffcutCB->setVisible(true);
  equalizeIlluminationCB->setChecked(blackWhiteOptions.normalizeIllumination());
  equalizeIlluminationCB->setVisible(color_mode != COLOR_GRAYSCALE);
  equalizeIlluminationColorCB->setChecked(colorCommonOptions.normalizeIllumination());
  equalizeIlluminationColorCB->setVisible(color_mode != BLACK_AND_WHITE);
  equalizeIlluminationColorCB->setEnabled(color_mode == COLOR_GRAYSCALE || blackWhiteOptions.normalizeIllumination());
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
  originalBackgroundCB->setChecked(m_splittingOptions.isOriginalBackgroundEnabled());
  colorForegroundRB->setEnabled(m_splittingOptions.isSplitOutput());
  bwForegroundRB->setEnabled(m_splittingOptions.isSplitOutput());
  originalBackgroundCB->setEnabled(m_splittingOptions.isSplitOutput()
                                   && (m_splittingOptions.getSplittingMode() == BLACK_AND_WHITE_FOREGROUND));

  thresholdMethodBox->setCurrentIndex((int) blackWhiteOptions.getBinarizationMethod());
  binarizationOptions->setCurrentIndex((int) blackWhiteOptions.getBinarizationMethod());

  fillingOptions->setVisible(color_mode != BLACK_AND_WHITE);
  fillingColorBox->setCurrentIndex((int) colorCommonOptions.getFillingColor());

  colorSegmentationCB->setVisible(threshold_options_visible);
  segmenterOptionsWidget->setVisible(threshold_options_visible);
  segmenterOptionsWidget->setEnabled(blackWhiteOptions.getColorSegmenterOptions().isEnabled());
  if (threshold_options_visible) {
    posterizeCB->setEnabled(blackWhiteOptions.getColorSegmenterOptions().isEnabled());
    posterizeOptionsWidget->setEnabled(blackWhiteOptions.getColorSegmenterOptions().isEnabled()
                                       && colorCommonOptions.getPosterizationOptions().isEnabled());
  } else {
    posterizeCB->setEnabled(true);
    posterizeOptionsWidget->setEnabled(colorCommonOptions.getPosterizationOptions().isEnabled());
  }
  colorSegmentationCB->setChecked(blackWhiteOptions.getColorSegmenterOptions().isEnabled());
  reduceNoiseSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getNoiseReduction());
  redAdjustmentSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getRedThresholdAdjustment());
  greenAdjustmentSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getGreenThresholdAdjustment());
  blueAdjustmentSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getBlueThresholdAdjustment());
  posterizeCB->setChecked(colorCommonOptions.getPosterizationOptions().isEnabled());
  posterizeLevelSB->setValue(colorCommonOptions.getPosterizationOptions().getLevel());
  posterizeNormalizationCB->setChecked(colorCommonOptions.getPosterizationOptions().isNormalizationEnabled());
  posterizeForceBwCB->setChecked(colorCommonOptions.getPosterizationOptions().isForceBlackAndWhite());

  if (picture_shape_visible) {
    const int picture_shape_idx = pictureShapeSelector->findData(m_pictureShapeOptions.getPictureShape());
    pictureShapeSelector->setCurrentIndex(picture_shape_idx);
    pictureShapeSensitivitySB->setValue(m_pictureShapeOptions.getSensitivity());
    pictureShapeSensitivityOptions->setVisible(m_pictureShapeOptions.getPictureShape() == RECTANGULAR_SHAPE);
    higherSearchSensitivityCB->setChecked(m_pictureShapeOptions.isHigherSearchSensitivity());
    higherSearchSensitivityCB->setVisible(m_pictureShapeOptions.getPictureShape() != OFF_SHAPE);
  }

  if (threshold_options_visible) {
    if (m_despeckleLevel != 0) {
      despeckleCB->setChecked(true);
      despeckleSlider->setValue(qRound(10 * m_despeckleLevel));
    } else {
      despeckleCB->setChecked(false);
    }
    despeckleSlider->setEnabled(m_despeckleLevel != 0);
    despeckleSlider->setToolTip(QString::number(0.1 * despeckleSlider->value()));

    for (int i = 0; i < binarizationOptions->count(); i++) {
      auto* widget = dynamic_cast<BinarizationOptionsWidget*>(binarizationOptions->widget(i));
      widget->updateUi(m_pageId);
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

  if ((m_dewarpingOptions.dewarpingMode() == MANUAL) || (m_dewarpingOptions.dewarpingMode() == MARGINAL)) {
    QString dewarping_status = dewarpingStatusLabel->text();
    if (m_dewarpingOptions.needPostDeskew()) {
      const double deskew_angle = -std::round(m_dewarpingOptions.getPostDeskewAngle() * 100) / 100;
      dewarping_status += " (" + tr("deskew") + ": " + QString::number(deskew_angle) + QChar(0x00B0) + ")";
    } else {
      dewarping_status += " (" + tr("deskew disabled") + ")";
    }
    dewarpingStatusLabel->setText(dewarping_status);
  }

  depthPerceptionSlider->blockSignals(true);
  depthPerceptionSlider->setValue(qRound(m_depthPerception.value() * 10));
  depthPerceptionSlider->blockSignals(false);
}

void OptionsWidget::savitzkyGolaySmoothingToggled(bool checked) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setSavitzkyGolaySmoothingEnabled(checked);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);
  emit reloadRequested();
}

void OptionsWidget::morphologicalSmoothingToggled(bool checked) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setMorphologicalSmoothingEnabled(checked);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);
  emit reloadRequested();
}

void OptionsWidget::bwForegroundToggled(bool checked) {
  if (!checked) {
    return;
  }

  originalBackgroundCB->setEnabled(checked);

  m_splittingOptions.setSplittingMode(BLACK_AND_WHITE_FOREGROUND);
  m_settings->setSplittingOptions(m_pageId, m_splittingOptions);
  emit reloadRequested();
}

void OptionsWidget::colorForegroundToggled(bool checked) {
  if (!checked) {
    return;
  }

  originalBackgroundCB->setEnabled(!checked);

  m_splittingOptions.setSplittingMode(COLOR_FOREGROUND);
  m_settings->setSplittingOptions(m_pageId, m_splittingOptions);
  emit reloadRequested();
}

void OptionsWidget::splittingToggled(bool checked) {
  m_splittingOptions.setSplitOutput(checked);

  bwForegroundRB->setEnabled(checked);
  colorForegroundRB->setEnabled(checked);
  originalBackgroundCB->setEnabled(checked && bwForegroundRB->isChecked());

  m_settings->setSplittingOptions(m_pageId, m_splittingOptions);
  emit reloadRequested();
}

void OptionsWidget::originalBackgroundToggled(bool checked) {
  m_splittingOptions.setOriginalBackgroundEnabled(checked);

  m_settings->setSplittingOptions(m_pageId, m_splittingOptions);
  emit reloadRequested();
}

void OptionsWidget::colorSegmentationToggled(bool checked) {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  BlackWhiteOptions::ColorSegmenterOptions segmenterOptions = blackWhiteOptions.getColorSegmenterOptions();
  segmenterOptions.setEnabled(checked);
  blackWhiteOptions.setColorSegmenterOptions(segmenterOptions);
  m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  segmenterOptionsWidget->setEnabled(checked);
  if ((m_colorParams.colorMode() == BLACK_AND_WHITE) || (m_colorParams.colorMode() == MIXED)) {
    posterizeCB->setEnabled(checked);
    posterizeOptionsWidget->setEnabled(checked && posterizeCB->isChecked());
  }

  emit reloadRequested();
}

void OptionsWidget::reduceNoiseChanged(int value) {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  BlackWhiteOptions::ColorSegmenterOptions segmenterOptions = blackWhiteOptions.getColorSegmenterOptions();
  segmenterOptions.setNoiseReduction(value);
  blackWhiteOptions.setColorSegmenterOptions(segmenterOptions);
  m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedReloadRequest.start(750);
}

void OptionsWidget::redAdjustmentChanged(int value) {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  BlackWhiteOptions::ColorSegmenterOptions segmenterOptions = blackWhiteOptions.getColorSegmenterOptions();
  segmenterOptions.setRedThresholdAdjustment(value);
  blackWhiteOptions.setColorSegmenterOptions(segmenterOptions);
  m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedReloadRequest.start(750);
}

void OptionsWidget::greenAdjustmentChanged(int value) {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  BlackWhiteOptions::ColorSegmenterOptions segmenterOptions = blackWhiteOptions.getColorSegmenterOptions();
  segmenterOptions.setGreenThresholdAdjustment(value);
  blackWhiteOptions.setColorSegmenterOptions(segmenterOptions);
  m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedReloadRequest.start(750);
}

void OptionsWidget::blueAdjustmentChanged(int value) {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  BlackWhiteOptions::ColorSegmenterOptions segmenterOptions = blackWhiteOptions.getColorSegmenterOptions();
  segmenterOptions.setBlueThresholdAdjustment(value);
  blackWhiteOptions.setColorSegmenterOptions(segmenterOptions);
  m_colorParams.setBlackWhiteOptions(blackWhiteOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedReloadRequest.start(750);
}

void OptionsWidget::posterizeToggled(bool checked) {
  ColorCommonOptions colorCommonOptions = m_colorParams.colorCommonOptions();
  ColorCommonOptions::PosterizationOptions posterizationOptions = colorCommonOptions.getPosterizationOptions();
  posterizationOptions.setEnabled(checked);
  colorCommonOptions.setPosterizationOptions(posterizationOptions);
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  posterizeOptionsWidget->setEnabled(checked);

  emit reloadRequested();
}

void OptionsWidget::posterizeLevelChanged(int value) {
  ColorCommonOptions colorCommonOptions = m_colorParams.colorCommonOptions();
  ColorCommonOptions::PosterizationOptions posterizationOptions = colorCommonOptions.getPosterizationOptions();
  posterizationOptions.setLevel(value);
  colorCommonOptions.setPosterizationOptions(posterizationOptions);
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedReloadRequest.start(750);
}

void OptionsWidget::posterizeNormalizationToggled(bool checked) {
  ColorCommonOptions colorCommonOptions = m_colorParams.colorCommonOptions();
  ColorCommonOptions::PosterizationOptions posterizationOptions = colorCommonOptions.getPosterizationOptions();
  posterizationOptions.setNormalizationEnabled(checked);
  colorCommonOptions.setPosterizationOptions(posterizationOptions);
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  emit reloadRequested();
}

void OptionsWidget::posterizeForceBwToggled(bool checked) {
  ColorCommonOptions colorCommonOptions = m_colorParams.colorCommonOptions();
  ColorCommonOptions::PosterizationOptions posterizationOptions = colorCommonOptions.getPosterizationOptions();
  posterizationOptions.setForceBlackAndWhite(checked);
  colorCommonOptions.setPosterizationOptions(posterizationOptions);
  m_colorParams.setColorCommonOptions(colorCommonOptions);
  m_settings->setColorParams(m_pageId, m_colorParams);

  emit reloadRequested();
}

void OptionsWidget::updateBinarizationOptionsDisplay(int idx) {
  for (int i = 0; i < binarizationOptions->count(); i++) {
    QWidget* currentWidget = binarizationOptions->widget(i);
    currentWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    currentWidget->resize(0, 0);

    disconnect(currentWidget, SIGNAL(stateChanged()), this, SLOT(binarizationSettingsChanged()));
  }

  QWidget* widget = binarizationOptions->widget(idx);
  widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  widget->adjustSize();
  binarizationOptions->adjustSize();

  connect(widget, SIGNAL(stateChanged()), this, SLOT(binarizationSettingsChanged()));
}

void OptionsWidget::addBinarizationOptionsWidget(BinarizationOptionsWidget* widget) {
  widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  binarizationOptions->addWidget(widget);
}

void OptionsWidget::sendReloadRequested() {
  emit reloadRequested();
}

#define CONNECT(...) m_connectionList.push_back(connect(__VA_ARGS__))

void OptionsWidget::setupUiConnections() {
  CONNECT(changeDpiButton, SIGNAL(clicked()), this, SLOT(changeDpiButtonClicked()));
  CONNECT(colorModeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(colorModeChanged(int)));
  CONNECT(thresholdMethodBox, SIGNAL(currentIndexChanged(int)), this, SLOT(thresholdMethodChanged(int)));
  CONNECT(fillingColorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(fillingColorChanged(int)));
  CONNECT(pictureShapeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(pictureShapeChanged(int)));
  CONNECT(pictureShapeSensitivitySB, SIGNAL(valueChanged(int)), this, SLOT(pictureShapeSensitivityChanged(int)));
  CONNECT(higherSearchSensitivityCB, SIGNAL(clicked(bool)), this, SLOT(higherSearchSensivityToggled(bool)));

  CONNECT(colorSegmentationCB, SIGNAL(clicked(bool)), this, SLOT(colorSegmentationToggled(bool)));
  CONNECT(reduceNoiseSB, SIGNAL(valueChanged(int)), this, SLOT(reduceNoiseChanged(int)));
  CONNECT(redAdjustmentSB, SIGNAL(valueChanged(int)), this, SLOT(redAdjustmentChanged(int)));
  CONNECT(greenAdjustmentSB, SIGNAL(valueChanged(int)), this, SLOT(greenAdjustmentChanged(int)));
  CONNECT(blueAdjustmentSB, SIGNAL(valueChanged(int)), this, SLOT(blueAdjustmentChanged(int)));
  CONNECT(posterizeCB, SIGNAL(clicked(bool)), this, SLOT(posterizeToggled(bool)));
  CONNECT(posterizeLevelSB, SIGNAL(valueChanged(int)), this, SLOT(posterizeLevelChanged(int)));
  CONNECT(posterizeNormalizationCB, SIGNAL(clicked(bool)), this, SLOT(posterizeNormalizationToggled(bool)));
  CONNECT(posterizeForceBwCB, SIGNAL(clicked(bool)), this, SLOT(posterizeForceBwToggled(bool)));

  CONNECT(fillMarginsCB, SIGNAL(clicked(bool)), this, SLOT(fillMarginsToggled(bool)));
  CONNECT(fillOffcutCB, SIGNAL(clicked(bool)), this, SLOT(fillOffcutToggled(bool)));
  CONNECT(equalizeIlluminationCB, SIGNAL(clicked(bool)), this, SLOT(equalizeIlluminationToggled(bool)));
  CONNECT(equalizeIlluminationColorCB, SIGNAL(clicked(bool)), this, SLOT(equalizeIlluminationColorToggled(bool)));
  CONNECT(savitzkyGolaySmoothingCB, SIGNAL(clicked(bool)), this, SLOT(savitzkyGolaySmoothingToggled(bool)));
  CONNECT(morphologicalSmoothingCB, SIGNAL(clicked(bool)), this, SLOT(morphologicalSmoothingToggled(bool)));
  CONNECT(splittingCB, SIGNAL(clicked(bool)), this, SLOT(splittingToggled(bool)));
  CONNECT(bwForegroundRB, SIGNAL(clicked(bool)), this, SLOT(bwForegroundToggled(bool)));
  CONNECT(colorForegroundRB, SIGNAL(clicked(bool)), this, SLOT(colorForegroundToggled(bool)));
  CONNECT(originalBackgroundCB, SIGNAL(clicked(bool)), this, SLOT(originalBackgroundToggled(bool)));
  CONNECT(applyColorsButton, SIGNAL(clicked()), this, SLOT(applyColorsButtonClicked()));

  CONNECT(applySplittingButton, SIGNAL(clicked()), this, SLOT(applySplittingButtonClicked()));

  CONNECT(changeDewarpingButton, SIGNAL(clicked()), this, SLOT(changeDewarpingButtonClicked()));

  CONNECT(applyDepthPerceptionButton, SIGNAL(clicked()), this, SLOT(applyDepthPerceptionButtonClicked()));

  CONNECT(despeckleCB, SIGNAL(clicked(bool)), this, SLOT(despeckleToggled(bool)));
  CONNECT(despeckleSlider, SIGNAL(sliderReleased()), this, SLOT(despeckleSliderReleased()));
  CONNECT(despeckleSlider, SIGNAL(valueChanged(int)), this, SLOT(despeckleSliderValueChanged(int)));
  CONNECT(applyDespeckleButton, SIGNAL(clicked()), this, SLOT(applyDespeckleButtonClicked()));
  CONNECT(depthPerceptionSlider, SIGNAL(valueChanged(int)), this, SLOT(depthPerceptionChangedSlot(int)));
  CONNECT(&m_delayedReloadRequest, SIGNAL(timeout()), this, SLOT(sendReloadRequested()));

  CONNECT(blackOnWhiteCB, SIGNAL(clicked(bool)), this, SLOT(blackOnWhiteToggled(bool)));
  CONNECT(applyProcessingOptionsButton, SIGNAL(clicked()), this, SLOT(applyProcessingParamsClicked()));
}

#undef CONNECT

void OptionsWidget::removeUiConnections() {
  for (const auto& connection : m_connectionList) {
    disconnect(connection);
  }
  m_connectionList.clear();
}

ImageViewTab OptionsWidget::lastTab() const {
  return m_lastTab;
}

const DepthPerception& OptionsWidget::depthPerception() const {
  return m_depthPerception;
}

void OptionsWidget::blackOnWhiteToggled(bool value) {
  m_settings->setBlackOnWhite(m_pageId, value);
  OutputProcessingParams processingParams = m_settings->getOutputProcessingParams(m_pageId);
  processingParams.setBlackOnWhiteSetManually(true);
  m_settings->setOutputProcessingParams(m_pageId, processingParams);

  emit reloadRequested();
}

void OptionsWidget::applyProcessingParamsClicked() {
  auto* dialog = new ApplyColorsDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Processing Settings"));
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&)), this,
          SLOT(applyProcessingParamsConfirmed(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::applyProcessingParamsConfirmed(const std::set<PageId>& pages) {
  for (const PageId& page_id : pages) {
    m_settings->setBlackOnWhite(page_id, m_settings->getParams(m_pageId).isBlackOnWhite());
    OutputProcessingParams processingParams = m_settings->getOutputProcessingParams(page_id);
    processingParams.setBlackOnWhiteSetManually(true);
    m_settings->setOutputProcessingParams(page_id, processingParams);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }

  if (pages.find(m_pageId) != pages.end()) {
    emit reloadRequested();
  }
}

void OptionsWidget::updateProcessingDisplay() {
  blackOnWhiteCB->setChecked(m_settings->getParams(m_pageId).isBlackOnWhite());
}
}  // namespace output