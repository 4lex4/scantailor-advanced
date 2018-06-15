
#include "DefaultParamsDialog.h"
#include <filters/output/ColorParams.h>
#include <filters/output/DewarpingOptions.h>
#include <filters/output/PictureShapeOptions.h>
#include <filters/page_split/LayoutType.h>
#include <foundation/ScopedIncDec.h>
#include <QLineEdit>
#include <QtCore/QSettings>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolTip>
#include <cassert>
#include <memory>
#include "DefaultParamsProvider.h"
#include "UnitsProvider.h"
#include "Utils.h"

using namespace page_split;
using namespace output;
using namespace page_layout;

DefaultParamsDialog::DefaultParamsDialog(QWidget* parent)
    : QDialog(parent),
      leftRightLinkEnabled(true),
      topBottomLinkEnabled(true),
      ignoreMarginChanges(0),
      currentUnits(MILLIMETRES),
      ignoreProfileChanges(0) {
  setupUi(this);

  layoutModeCB->addItem(tr("Auto"), MODE_AUTO);
  layoutModeCB->addItem(tr("Manual"), MODE_MANUAL);

  colorModeSelector->addItem(tr("Black and White"), BLACK_AND_WHITE);
  colorModeSelector->addItem(tr("Color / Grayscale"), COLOR_GRAYSCALE);
  colorModeSelector->addItem(tr("Mixed"), MIXED);

  fillingColorBox->addItem(tr("Background"), FILL_BACKGROUND);
  fillingColorBox->addItem(tr("White"), FILL_WHITE);

  thresholdMethodBox->addItem(tr("Otsu"), OTSU);
  thresholdMethodBox->addItem(tr("Sauvola"), SAUVOLA);
  thresholdMethodBox->addItem(tr("Wolf"), WOLF);

  pictureShapeSelector->addItem(tr("Off"), OFF_SHAPE);
  pictureShapeSelector->addItem(tr("Free"), FREE_SHAPE);
  pictureShapeSelector->addItem(tr("Rectangular"), RECTANGULAR_SHAPE);

  dpiSelector->addItem("300", "300");
  dpiSelector->addItem("400", "400");
  dpiSelector->addItem("600", "600");
  customDpiItemIdx = dpiSelector->count();
  customDpiValue = "200";
  dpiSelector->addItem(tr("Custom"), customDpiValue);

  dewarpingModeCB->addItem(tr("Off"), OFF);
  dewarpingModeCB->addItem(tr("Auto"), AUTO);
  dewarpingModeCB->addItem(tr("Manual"), MANUAL);
  dewarpingModeCB->addItem(tr("Marginal"), MARGINAL);

  reservedProfileNames.insert("Default");
  reservedProfileNames.insert("Source");
  reservedProfileNames.insert("Custom");

  profileCB->addItem(tr("Default"), "Default");
  profileCB->addItem(tr("Source"), "Source");
  std::unique_ptr<std::list<QString>> profileList = profileManager.getProfileList();
  for (const QString& profileName : *profileList) {
    if (!isProfileNameReserved(profileName)) {
      profileCB->addItem(profileName, profileName);
    }
  }
  customProfileItemIdx = profileCB->count();
  profileCB->addItem(tr("Custom"), "Custom");

  reservedProfileNames.insert(profileCB->itemText(profileCB->findData("Default")));
  reservedProfileNames.insert(profileCB->itemText(profileCB->findData("Source")));
  reservedProfileNames.insert(profileCB->itemText(profileCB->findData("Custom")));

  chainIcon.addPixmap(QPixmap(QString::fromLatin1(":/icons/stock-vchain-24.png")));
  brokenChainIcon.addPixmap(QPixmap(QString::fromLatin1(":/icons/stock-vchain-broken-24.png")));
  setLinkButtonLinked(topBottomLink, topBottomLinkEnabled);
  setLinkButtonLinked(leftRightLink, leftRightLinkEnabled);

  Utils::mapSetValue(alignmentByButton, alignTopLeftBtn, Alignment(Alignment::TOP, Alignment::LEFT));
  Utils::mapSetValue(alignmentByButton, alignTopBtn, Alignment(Alignment::TOP, Alignment::HCENTER));
  Utils::mapSetValue(alignmentByButton, alignTopRightBtn, Alignment(Alignment::TOP, Alignment::RIGHT));
  Utils::mapSetValue(alignmentByButton, alignLeftBtn, Alignment(Alignment::VCENTER, Alignment::LEFT));
  Utils::mapSetValue(alignmentByButton, alignCenterBtn, Alignment(Alignment::VCENTER, Alignment::HCENTER));
  Utils::mapSetValue(alignmentByButton, alignRightBtn, Alignment(Alignment::VCENTER, Alignment::RIGHT));
  Utils::mapSetValue(alignmentByButton, alignBottomLeftBtn, Alignment(Alignment::BOTTOM, Alignment::LEFT));
  Utils::mapSetValue(alignmentByButton, alignBottomBtn, Alignment(Alignment::BOTTOM, Alignment::HCENTER));
  Utils::mapSetValue(alignmentByButton, alignBottomRightBtn, Alignment(Alignment::BOTTOM, Alignment::RIGHT));

  alignmentButtonGroup = std::make_unique<QButtonGroup>(this);
  for (const auto& buttonAndAlignment : alignmentByButton) {
    alignmentButtonGroup->addButton(buttonAndAlignment.first);
  }

  darkerThresholdLink->setText(Utils::richTextForLink(darkerThresholdLink->text()));
  lighterThresholdLink->setText(Utils::richTextForLink(lighterThresholdLink->text()));
  thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));

  thresholdSlider->setMinimum(-100);
  thresholdSlider->setMaximum(100);
  thresholLabel->setText(QString::number(thresholdSlider->value()));

  depthPerceptionSlider->setMinimum(qRound(DepthPerception::minValue() * 10));
  depthPerceptionSlider->setMaximum(qRound(DepthPerception::maxValue() * 10));

  despeckleSlider->setMinimum(qRound(1.0 * 10));
  despeckleSlider->setMaximum(qRound(3.0 * 10));

  const int index = profileCB->findData(DefaultParamsProvider::getInstance()->getProfileName());
  if (index != -1) {
    profileCB->setCurrentIndex(index);
  }
  profileChanged(profileCB->currentIndex());

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(commitChanges()));
}

void DefaultParamsDialog::updateFixOrientationDisplay(const DefaultParams::FixOrientationParams& params) {
  orthogonalRotation = params.getImageRotation();
  setRotationPixmap();
}

void DefaultParamsDialog::updatePageSplitDisplay(const DefaultParams::PageSplitParams& params) {
  LayoutType layoutType = params.getLayoutType();
  if (layoutType == AUTO_LAYOUT_TYPE) {
    layoutModeCB->setCurrentIndex(layoutModeCB->findData(static_cast<int>(MODE_AUTO)));
    pageLayoutGroup->setEnabled(false);
  } else {
    layoutModeCB->setCurrentIndex(layoutModeCB->findData(static_cast<int>(MODE_MANUAL)));
    pageLayoutGroup->setEnabled(true);
  }

  switch (layoutType) {
    case AUTO_LAYOUT_TYPE:
      // Uncheck all buttons.  Can only be done
      // by playing with exclusiveness.
      twoPagesBtn->setChecked(true);
      twoPagesBtn->setAutoExclusive(false);
      twoPagesBtn->setChecked(false);
      twoPagesBtn->setAutoExclusive(true);
      break;
    case SINGLE_PAGE_UNCUT:
      singlePageUncutBtn->setChecked(true);
      break;
    case PAGE_PLUS_OFFCUT:
      pagePlusOffcutBtn->setChecked(true);
      break;
    case TWO_PAGES:
      twoPagesBtn->setChecked(true);
      break;
  }
}

void DefaultParamsDialog::updateDeskewDisplay(const DefaultParams::DeskewParams& params) {
  AutoManualMode mode = params.getMode();
  if (mode == MODE_AUTO) {
    deskewAutoBtn->setChecked(true);
  } else {
    deskewManualBtn->setChecked(true);
  }
  angleSpinBox->setEnabled(mode == MODE_MANUAL);
  angleSpinBox->setValue(params.getDeskewAngleDeg());
}

void DefaultParamsDialog::updateSelectContentDisplay(const DefaultParams::SelectContentParams& params) {
  if (params.isPageDetectEnabled()) {
    pageDetectOptions->setEnabled(true);
    AutoManualMode pageDetectMode = params.getPageDetectMode();
    fineTuneBtn->setEnabled(pageDetectMode == MODE_AUTO);
    dimensionsWidget->setEnabled(pageDetectMode == MODE_MANUAL);
    if (pageDetectMode == MODE_AUTO) {
      pageDetectAutoBtn->setChecked(true);
    } else {
      pageDetectManualBtn->setChecked(true);
    }
  } else {
    pageDetectOptions->setEnabled(false);
    pageDetectDisableBtn->setChecked(true);
  }

  if (params.isContentDetectEnabled()) {
    contentDetectAutoBtn->setChecked(true);
  } else {
    contentDetectDisableBtn->setChecked(true);
  }

  QSizeF pageRectSize = params.getPageRectSize();
  widthSpinBox->setValue(pageRectSize.width());
  heightSpinBox->setValue(pageRectSize.height());
  fineTuneBtn->setChecked(params.isFineTuneCorners());
}

void DefaultParamsDialog::updatePageLayoutDisplay(const DefaultParams::PageLayoutParams& params) {
  autoMargins->setChecked(params.isAutoMargins());
  marginsWidget->setEnabled(!params.isAutoMargins());

  const Margins& margins = params.getHardMargins();
  topMarginSpinBox->setValue(margins.top());
  rightMarginSpinBox->setValue(margins.right());
  bottomMarginSpinBox->setValue(margins.bottom());
  leftMarginSpinBox->setValue(margins.left());

  topBottomLinkEnabled = (margins.top() == margins.bottom());
  leftRightLinkEnabled = (margins.left() == margins.right());
  setLinkButtonLinked(topBottomLink, topBottomLinkEnabled);
  setLinkButtonLinked(leftRightLink, leftRightLinkEnabled);

  const Alignment& alignment = params.getAlignment();
  if (alignment.isAuto()) {
    alignmentMode->setCurrentIndex(0);
    autoAlignSettingsGroup->setVisible(true);
    autoVerticalAligningCB->setChecked(alignment.isAutoVertical());
    autoHorizontalAligningCB->setChecked(alignment.isAutoHorizontal());
  } else if (alignment.isOriginal()) {
    alignmentMode->setCurrentIndex(2);
    autoAlignSettingsGroup->setVisible(true);
    autoVerticalAligningCB->setChecked(alignment.isAutoVertical());
    autoHorizontalAligningCB->setChecked(alignment.isAutoHorizontal());
  } else {
    alignmentMode->setCurrentIndex(1);
    autoAlignSettingsGroup->setVisible(false);
  }
  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();

  alignWithOthersCB->setChecked(!alignment.isNull());

  for (const auto& kv : alignmentByButton) {
    if (alignment.isAuto() || alignment.isOriginal()) {
      if (!alignment.isAutoHorizontal() && (kv.second.vertical() == Alignment::VCENTER)
          && (kv.second.horizontal() == alignment.horizontal())) {
        kv.first->setChecked(true);
        break;
      } else if (!alignment.isAutoVertical() && (kv.second.horizontal() == Alignment::HCENTER)
                 && (kv.second.vertical() == alignment.vertical())) {
        kv.first->setChecked(true);
        break;
      }
    } else if (kv.second == alignment) {
      kv.first->setChecked(true);
      break;
    }
  }
}

void DefaultParamsDialog::updateOutputDisplay(const DefaultParams::OutputParams& params) {
  const ColorParams& colorParams = params.getColorParams();
  colorModeSelector->setCurrentIndex(colorModeSelector->findData(colorParams.colorMode()));

  const ColorCommonOptions& colorCommonOptions = colorParams.colorCommonOptions();
  const BlackWhiteOptions& blackWhiteOptions = colorParams.blackWhiteOptions();
  fillMarginsCB->setChecked(colorCommonOptions.fillMargins());
  fillOffcutCB->setChecked(colorCommonOptions.fillOffcut());
  equalizeIlluminationCB->setChecked(blackWhiteOptions.normalizeIllumination());
  equalizeIlluminationColorCB->setChecked(colorCommonOptions.normalizeIllumination());
  savitzkyGolaySmoothingCB->setChecked(blackWhiteOptions.isSavitzkyGolaySmoothingEnabled());
  morphologicalSmoothingCB->setChecked(blackWhiteOptions.isMorphologicalSmoothingEnabled());

  fillingColorBox->setCurrentIndex(fillingColorBox->findData(colorCommonOptions.getFillingColor()));

  colorSegmentationCB->setChecked(blackWhiteOptions.getColorSegmenterOptions().isEnabled());
  reduceNoiseSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getNoiseReduction());
  redAdjustmentSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getRedThresholdAdjustment());
  greenAdjustmentSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getGreenThresholdAdjustment());
  blueAdjustmentSB->setValue(blackWhiteOptions.getColorSegmenterOptions().getBlueThresholdAdjustment());
  posterizeCB->setChecked(colorCommonOptions.getPosterizationOptions().isEnabled());
  posterizeLevelSB->setValue(colorCommonOptions.getPosterizationOptions().getLevel());
  posterizeNormalizationCB->setChecked(colorCommonOptions.getPosterizationOptions().isNormalizationEnabled());
  posterizeForceBwCB->setChecked(colorCommonOptions.getPosterizationOptions().isForceBlackAndWhite());

  thresholdMethodBox->setCurrentIndex(thresholdMethodBox->findData(blackWhiteOptions.getBinarizationMethod()));
  thresholdSlider->setValue(blackWhiteOptions.thresholdAdjustment());
  thresholLabel->setText(QString::number(thresholdSlider->value()));
  sauvolaWindowSize->setValue(blackWhiteOptions.getWindowSize());
  wolfWindowSize->setValue(blackWhiteOptions.getWindowSize());
  sauvolaCoef->setValue(blackWhiteOptions.getSauvolaCoef());
  lowerBound->setValue(blackWhiteOptions.getWolfLowerBound());
  upperBound->setValue(blackWhiteOptions.getWolfUpperBound());
  wolfCoef->setValue(blackWhiteOptions.getWolfCoef());

  const PictureShapeOptions& pictureShapeOptions = params.getPictureShapeOptions();
  pictureShapeSelector->setCurrentIndex(pictureShapeSelector->findData(pictureShapeOptions.getPictureShape()));
  pictureShapeSensitivitySB->setValue(pictureShapeOptions.getSensitivity());
  higherSearchSensitivityCB->setChecked(pictureShapeOptions.isHigherSearchSensitivity());

  int dpiIndex = dpiSelector->findData(QString::number(params.getDpi().vertical()));
  if (dpiIndex != -1) {
    dpiSelector->setCurrentIndex(dpiIndex);
  } else {
    dpiSelector->setCurrentIndex(customDpiItemIdx);
    customDpiValue = QString::number(params.getDpi().vertical());
  }

  const SplittingOptions& splittingOptions = params.getSplittingOptions();
  splittingCB->setChecked(splittingOptions.isSplitOutput());
  switch (splittingOptions.getSplittingMode()) {
    case BLACK_AND_WHITE_FOREGROUND:
      bwForegroundRB->setChecked(true);
      break;
    case COLOR_FOREGROUND:
      colorForegroundRB->setChecked(true);
      break;
  }
  originalBackgroundCB->setChecked(splittingOptions.isOriginalBackground());

  const double despeckleLevel = params.getDespeckleLevel();
  if (despeckleLevel != 0) {
    despeckleCB->setChecked(true);
    despeckleSlider->setValue(qRound(10 * despeckleLevel));
  } else {
    despeckleCB->setChecked(false);
  }
  despeckleSlider->setToolTip(QString::number(0.1 * despeckleSlider->value()));

  dewarpingModeCB->setCurrentIndex(dewarpingModeCB->findData(params.getDewarpingOptions().dewarpingMode()));
  dewarpingPostDeskewCB->setChecked(params.getDewarpingOptions().needPostDeskew());
  depthPerceptionSlider->setValue(qRound(params.getDepthPerception().value() * 10));

  // update the display
  colorModeChanged(colorModeSelector->currentIndex());
  thresholdMethodChanged(thresholdMethodBox->currentIndex());
  pictureShapeChanged(pictureShapeSelector->currentIndex());
  equalizeIlluminationToggled(equalizeIlluminationCB->isChecked());
  splittingToggled(splittingCB->isChecked());
  dpiSelectionChanged(dpiSelector->currentIndex());
  despeckleToggled(despeckleCB->isChecked());
}

void DefaultParamsDialog::setupUiConnections() {
  connect(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
  connect(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
  connect(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
  connect(layoutModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(layoutModeChanged(int)));
  connect(deskewAutoBtn, SIGNAL(toggled(bool)), this, SLOT(deskewModeChanged(bool)));
  connect(pageDetectAutoBtn, SIGNAL(pressed()), this, SLOT(pageDetectAutoToggled()));
  connect(pageDetectManualBtn, SIGNAL(pressed()), this, SLOT(pageDetectManualToggled()));
  connect(pageDetectDisableBtn, SIGNAL(pressed()), this, SLOT(pageDetectDisableToggled()));
  connect(autoMargins, SIGNAL(toggled(bool)), this, SLOT(autoMarginsToggled(bool)));
  connect(alignmentMode, SIGNAL(currentIndexChanged(int)), this, SLOT(alignmentModeChanged(int)));
  connect(alignWithOthersCB, SIGNAL(toggled(bool)), this, SLOT(alignWithOthersToggled(bool)));
  connect(topBottomLink, SIGNAL(clicked()), this, SLOT(topBottomLinkClicked()));
  connect(leftRightLink, SIGNAL(clicked()), this, SLOT(leftRightLinkClicked()));
  connect(topMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  connect(bottomMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  connect(leftMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  connect(rightMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  connect(colorModeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(colorModeChanged(int)));
  connect(thresholdMethodBox, SIGNAL(currentIndexChanged(int)), this, SLOT(thresholdMethodChanged(int)));
  connect(pictureShapeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(pictureShapeChanged(int)));
  connect(equalizeIlluminationCB, SIGNAL(clicked(bool)), this, SLOT(equalizeIlluminationToggled(bool)));
  connect(splittingCB, SIGNAL(clicked(bool)), this, SLOT(splittingToggled(bool)));
  connect(bwForegroundRB, SIGNAL(clicked(bool)), this, SLOT(bwForegroundToggled(bool)));
  connect(colorForegroundRB, SIGNAL(clicked(bool)), this, SLOT(colorForegroundToggled(bool)));
  connect(lighterThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setLighterThreshold()));
  connect(darkerThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setDarkerThreshold()));
  connect(thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(thresholdSliderValueChanged(int)));
  connect(neutralThresholdBtn, SIGNAL(clicked()), this, SLOT(setNeutralThreshold()));
  connect(dpiSelector, SIGNAL(activated(int)), this, SLOT(dpiSelectionChanged(int)));
  connect(dpiSelector, SIGNAL(editTextChanged(const QString&)), this, SLOT(dpiEditTextChanged(const QString&)));
  connect(depthPerceptionSlider, SIGNAL(valueChanged(int)), this, SLOT(depthPerceptionChangedSlot(int)));
  connect(profileCB, SIGNAL(currentIndexChanged(int)), this, SLOT(profileChanged(int)));
  connect(profileSaveButton, SIGNAL(pressed()), this, SLOT(profileSavePressed()));
  connect(profileDeleteButton, SIGNAL(pressed()), this, SLOT(profileDeletePressed()));
  connect(colorSegmentationCB, SIGNAL(clicked(bool)), this, SLOT(colorSegmentationToggled(bool)));
  connect(posterizeCB, SIGNAL(clicked(bool)), this, SLOT(posterizeToggled(bool)));
  connect(autoHorizontalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoHorizontalAligningToggled(bool)));
  connect(autoVerticalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoVerticalAligningToggled(bool)));
  connect(despeckleCB, SIGNAL(clicked(bool)), this, SLOT(despeckleToggled(bool)));
  connect(despeckleSlider, SIGNAL(valueChanged(int)), this, SLOT(despeckleSliderValueChanged(int)));
}

void DefaultParamsDialog::removeUiConnections() {
  disconnect(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
  disconnect(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
  disconnect(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
  disconnect(layoutModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(layoutModeChanged(int)));
  disconnect(deskewAutoBtn, SIGNAL(toggled(bool)), this, SLOT(deskewModeChanged(bool)));
  disconnect(pageDetectAutoBtn, SIGNAL(pressed()), this, SLOT(pageDetectAutoToggled()));
  disconnect(pageDetectManualBtn, SIGNAL(pressed()), this, SLOT(pageDetectManualToggled()));
  disconnect(pageDetectDisableBtn, SIGNAL(pressed()), this, SLOT(pageDetectDisableToggled()));
  disconnect(autoMargins, SIGNAL(toggled(bool)), this, SLOT(autoMarginsToggled(bool)));
  disconnect(alignmentMode, SIGNAL(currentIndexChanged(int)), this, SLOT(alignmentModeChanged(int)));
  disconnect(alignWithOthersCB, SIGNAL(toggled(bool)), this, SLOT(alignWithOthersToggled(bool)));
  disconnect(topBottomLink, SIGNAL(clicked()), this, SLOT(topBottomLinkClicked()));
  disconnect(leftRightLink, SIGNAL(clicked()), this, SLOT(leftRightLinkClicked()));
  disconnect(topMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  disconnect(bottomMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  disconnect(leftMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  disconnect(rightMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  disconnect(colorModeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(colorModeChanged(int)));
  disconnect(thresholdMethodBox, SIGNAL(currentIndexChanged(int)), this, SLOT(thresholdMethodChanged(int)));
  disconnect(pictureShapeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(pictureShapeChanged(int)));
  disconnect(equalizeIlluminationCB, SIGNAL(clicked(bool)), this, SLOT(equalizeIlluminationToggled(bool)));
  disconnect(splittingCB, SIGNAL(clicked(bool)), this, SLOT(splittingToggled(bool)));
  disconnect(bwForegroundRB, SIGNAL(clicked(bool)), this, SLOT(bwForegroundToggled(bool)));
  disconnect(colorForegroundRB, SIGNAL(clicked(bool)), this, SLOT(colorForegroundToggled(bool)));
  disconnect(lighterThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setLighterThreshold()));
  disconnect(darkerThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setDarkerThreshold()));
  disconnect(thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(thresholdSliderValueChanged(int)));
  disconnect(neutralThresholdBtn, SIGNAL(clicked()), this, SLOT(setNeutralThreshold()));
  disconnect(dpiSelector, SIGNAL(activated(int)), this, SLOT(dpiSelectionChanged(int)));
  disconnect(dpiSelector, SIGNAL(editTextChanged(const QString&)), this, SLOT(dpiEditTextChanged(const QString&)));
  disconnect(depthPerceptionSlider, SIGNAL(valueChanged(int)), this, SLOT(depthPerceptionChangedSlot(int)));
  disconnect(profileCB, SIGNAL(currentIndexChanged(int)), this, SLOT(profileChanged(int)));
  disconnect(profileSaveButton, SIGNAL(pressed()), this, SLOT(profileSavePressed()));
  disconnect(profileDeleteButton, SIGNAL(pressed()), this, SLOT(profileDeletePressed()));
  disconnect(colorSegmentationCB, SIGNAL(clicked(bool)), this, SLOT(colorSegmentationToggled(bool)));
  disconnect(posterizeCB, SIGNAL(clicked(bool)), this, SLOT(posterizeToggled(bool)));
  disconnect(autoHorizontalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoHorizontalAligningToggled(bool)));
  disconnect(autoVerticalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoVerticalAligningToggled(bool)));
  disconnect(despeckleCB, SIGNAL(clicked(bool)), this, SLOT(despeckleToggled(bool)));
  disconnect(despeckleSlider, SIGNAL(valueChanged(int)), this, SLOT(despeckleSliderValueChanged(int)));
}

void DefaultParamsDialog::rotateLeft() {
  OrthogonalRotation rotation(orthogonalRotation);
  rotation.prevClockwiseDirection();
  setRotation(rotation);
}

void DefaultParamsDialog::rotateRight() {
  OrthogonalRotation rotation(orthogonalRotation);
  rotation.nextClockwiseDirection();
  setRotation(rotation);
}

void DefaultParamsDialog::resetRotation() {
  setRotation(OrthogonalRotation());
}

void DefaultParamsDialog::setRotation(const OrthogonalRotation& rotation) {
  if (rotation == orthogonalRotation) {
    return;
  }

  orthogonalRotation = rotation;
  setRotationPixmap();
}

void DefaultParamsDialog::setRotationPixmap() {
  const char* path = nullptr;

  switch (orthogonalRotation.toDegrees()) {
    case 0:
      path = ":/icons/big-up-arrow.png";
      break;
    case 90:
      path = ":/icons/big-right-arrow.png";
      break;
    case 180:
      path = ":/icons/big-down-arrow.png";
      break;
    case 270:
      path = ":/icons/big-left-arrow.png";
      break;
    default:
      assert(!"Unreachable");
  }

  rotationIndicator->setPixmap(QPixmap(path));
}

void DefaultParamsDialog::layoutModeChanged(const int idx) {
  const AutoManualMode mode = static_cast<AutoManualMode>(layoutModeCB->itemData(idx).toInt());
  if (mode == MODE_AUTO) {
    // Uncheck all buttons.  Can only be done
    // by playing with exclusiveness.
    twoPagesBtn->setChecked(true);
    twoPagesBtn->setAutoExclusive(false);
    twoPagesBtn->setChecked(false);
    twoPagesBtn->setAutoExclusive(true);
  } else {
    singlePageUncutBtn->setChecked(true);
  }
  pageLayoutGroup->setEnabled(mode == MODE_MANUAL);
}

void DefaultParamsDialog::deskewModeChanged(const bool auto_mode) {
  angleSpinBox->setEnabled(!auto_mode);
}

void DefaultParamsDialog::pageDetectAutoToggled() {
  pageDetectOptions->setEnabled(true);
  fineTuneBtn->setEnabled(true);
  dimensionsWidget->setEnabled(false);
}

void DefaultParamsDialog::pageDetectManualToggled() {
  pageDetectOptions->setEnabled(true);
  fineTuneBtn->setEnabled(false);
  dimensionsWidget->setEnabled(true);
}

void DefaultParamsDialog::pageDetectDisableToggled() {
  pageDetectOptions->setEnabled(false);
}

void DefaultParamsDialog::autoMarginsToggled(const bool checked) {
  marginsWidget->setEnabled(!checked);
}

void DefaultParamsDialog::alignmentModeChanged(const int idx) {
  autoAlignSettingsGroup->setVisible((idx == 0) || (idx == 2));
  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();
}

void DefaultParamsDialog::alignWithOthersToggled(const bool) {
  updateAlignmentButtonsEnabled();
}

void DefaultParamsDialog::colorModeChanged(const int idx) {
  const auto colorMode = static_cast<ColorMode>(colorModeSelector->itemData(idx).toInt());
  bool threshold_options_visible = false;
  bool picture_shape_visible = false;
  bool splitting_options_visible = false;
  switch (colorMode) {
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
  thresholdOptions->setEnabled(threshold_options_visible);
  pictureShapeOptions->setEnabled(picture_shape_visible);
  splittingOptions->setEnabled(splitting_options_visible);

  fillingOptions->setEnabled(colorMode != BLACK_AND_WHITE);

  equalizeIlluminationCB->setEnabled(colorMode != COLOR_GRAYSCALE);
  equalizeIlluminationColorCB->setEnabled(colorMode != BLACK_AND_WHITE);
  if ((colorMode == MIXED)) {
    if (equalizeIlluminationColorCB->isChecked()) {
      equalizeIlluminationColorCB->setChecked(equalizeIlluminationCB->isChecked());
    }
    equalizeIlluminationColorCB->setEnabled(equalizeIlluminationCB->isChecked());
  }
  savitzkyGolaySmoothingCB->setEnabled(colorMode != COLOR_GRAYSCALE);
  morphologicalSmoothingCB->setEnabled(colorMode != COLOR_GRAYSCALE);

  colorSegmentationCB->setEnabled(threshold_options_visible);
  segmenterOptionsWidget->setEnabled(threshold_options_visible);
  segmenterOptionsWidget->setEnabled(colorSegmentationCB->isChecked());
  if (threshold_options_visible) {
    posterizeCB->setEnabled(colorSegmentationCB->isChecked());
    posterizeOptionsWidget->setEnabled(colorSegmentationCB->isChecked() && posterizeCB->isChecked());
  } else {
    posterizeCB->setEnabled(true);
    posterizeOptionsWidget->setEnabled(posterizeCB->isChecked());
  }
}

void DefaultParamsDialog::thresholdMethodChanged(const int idx) {
  binarizationOptions->setCurrentIndex(idx);
}

void DefaultParamsDialog::pictureShapeChanged(const int idx) {
  const auto shapeMode = static_cast<PictureShape>(pictureShapeSelector->itemData(idx).toInt());
  pictureShapeSensitivityOptions->setEnabled(shapeMode == RECTANGULAR_SHAPE);
  higherSearchSensitivityCB->setEnabled(shapeMode != OFF_SHAPE);
}

void DefaultParamsDialog::equalizeIlluminationToggled(const bool checked) {
  const auto colorMode = static_cast<ColorMode>(colorModeSelector->currentData().toInt());
  if (colorMode == MIXED) {
    if (equalizeIlluminationColorCB->isChecked()) {
      equalizeIlluminationColorCB->setChecked(checked);
    }
    equalizeIlluminationColorCB->setEnabled(checked);
  }
}

void DefaultParamsDialog::splittingToggled(const bool checked) {
  bwForegroundRB->setEnabled(checked);
  colorForegroundRB->setEnabled(checked);
  originalBackgroundCB->setEnabled(checked && bwForegroundRB->isChecked());
}

void DefaultParamsDialog::bwForegroundToggled(bool checked) {
  if (!checked) {
    return;
  }
  originalBackgroundCB->setEnabled(checked);
}

void DefaultParamsDialog::colorForegroundToggled(bool checked) {
  if (!checked) {
    return;
  }
  originalBackgroundCB->setEnabled(!checked);
}

void DefaultParamsDialog::loadParams(const DefaultParams& params) {
  removeUiConnections();

  // must be done before updating the displays in order to set the precise of the spin boxes
  updateUnits(params.getUnits());

  updateFixOrientationDisplay(params.getFixOrientationParams());
  updatePageSplitDisplay(params.getPageSplitParams());
  updateDeskewDisplay(params.getDeskewParams());
  updateSelectContentDisplay(params.getSelectContentParams());
  updatePageLayoutDisplay(params.getPageLayoutParams());
  updateOutputDisplay(params.getOutputParams());

  setupUiConnections();
}

std::unique_ptr<DefaultParams> DefaultParamsDialog::buildParams() const {
  DefaultParams::FixOrientationParams fixOrientationParams(orthogonalRotation);

  LayoutType layoutType;
  if (layoutModeCB->currentData() == MODE_AUTO) {
    layoutType = AUTO_LAYOUT_TYPE;
  } else if (singlePageUncutBtn->isChecked()) {
    layoutType = SINGLE_PAGE_UNCUT;
  } else if (pagePlusOffcutBtn->isChecked()) {
    layoutType = PAGE_PLUS_OFFCUT;
  } else {
    layoutType = TWO_PAGES;
  }
  DefaultParams::PageSplitParams pageSplitParams(layoutType);

  DefaultParams::DeskewParams deskewParams(angleSpinBox->value(), deskewAutoBtn->isChecked() ? MODE_AUTO : MODE_MANUAL);

  DefaultParams::SelectContentParams selectContentParams(
      QSizeF(widthSpinBox->value(), heightSpinBox->value()), pageDetectManualBtn->isChecked() ? MODE_MANUAL : MODE_AUTO,
      !contentDetectDisableBtn->isChecked(), !pageDetectDisableBtn->isChecked(), fineTuneBtn->isChecked());

  Alignment alignment;
  switch (alignmentMode->currentIndex()) {
    case 0:
      if (autoVerticalAligningCB->isChecked()) {
        alignment.setVertical(Alignment::VAUTO);
      } else {
        alignment.setVertical(alignmentByButton.at(getCheckedAlignmentButton()).vertical());
      }
      if (autoHorizontalAligningCB->isChecked()) {
        alignment.setHorizontal(Alignment::HAUTO);
      } else {
        alignment.setHorizontal(alignmentByButton.at(getCheckedAlignmentButton()).horizontal());
      }
      break;
    case 1:
      alignment = alignmentByButton.at(getCheckedAlignmentButton());
      break;
    case 2:
      if (autoVerticalAligningCB->isChecked()) {
        alignment.setVertical(Alignment::VORIGINAL);
      } else {
        alignment.setVertical(alignmentByButton.at(getCheckedAlignmentButton()).vertical());
      }
      if (autoHorizontalAligningCB->isChecked()) {
        alignment.setHorizontal(Alignment::HORIGINAL);
      } else {
        alignment.setHorizontal(alignmentByButton.at(getCheckedAlignmentButton()).horizontal());
      }
      break;
    default:
      break;
  }
  alignment.setNull(!alignWithOthersCB->isChecked());
  DefaultParams::PageLayoutParams pageLayoutParams(Margins(leftMarginSpinBox->value(), topMarginSpinBox->value(),
                                                           rightMarginSpinBox->value(), bottomMarginSpinBox->value()),
                                                   alignment, autoMargins->isChecked());

  const int dpi
      = (dpiSelector->currentIndex() != customDpiItemIdx) ? dpiSelector->currentText().toInt() : customDpiValue.toInt();
  ColorParams colorParams;
  colorParams.setColorMode(static_cast<ColorMode>(colorModeSelector->currentData().toInt()));

  ColorCommonOptions colorCommonOptions;
  colorCommonOptions.setFillingColor(static_cast<FillingColor>(fillingColorBox->currentData().toInt()));
  colorCommonOptions.setFillMargins(fillMarginsCB->isChecked());
  colorCommonOptions.setFillOffcut(fillOffcutCB->isChecked());
  colorCommonOptions.setNormalizeIllumination(equalizeIlluminationColorCB->isChecked());
  ColorCommonOptions::PosterizationOptions posterizationOptions = colorCommonOptions.getPosterizationOptions();
  posterizationOptions.setEnabled(posterizeCB->isChecked());
  posterizationOptions.setLevel(posterizeLevelSB->value());
  posterizationOptions.setNormalizationEnabled(posterizeNormalizationCB->isChecked());
  posterizationOptions.setForceBlackAndWhite(posterizeForceBwCB->isChecked());
  colorCommonOptions.setPosterizationOptions(posterizationOptions);
  colorParams.setColorCommonOptions(colorCommonOptions);

  BlackWhiteOptions blackWhiteOptions;
  blackWhiteOptions.setNormalizeIllumination(equalizeIlluminationCB->isChecked());
  blackWhiteOptions.setSavitzkyGolaySmoothingEnabled(savitzkyGolaySmoothingCB->isChecked());
  blackWhiteOptions.setMorphologicalSmoothingEnabled(morphologicalSmoothingCB->isChecked());
  BinarizationMethod binarizationMethod = static_cast<BinarizationMethod>(thresholdMethodBox->currentData().toInt());
  blackWhiteOptions.setBinarizationMethod(binarizationMethod);
  blackWhiteOptions.setThresholdAdjustment(thresholdSlider->value());
  blackWhiteOptions.setSauvolaCoef(sauvolaCoef->value());
  if (binarizationMethod == SAUVOLA) {
    blackWhiteOptions.setWindowSize(sauvolaWindowSize->value());
  } else if (binarizationMethod == WOLF) {
    blackWhiteOptions.setWindowSize(wolfWindowSize->value());
  }
  blackWhiteOptions.setWolfCoef(wolfCoef->value());
  blackWhiteOptions.setWolfLowerBound(upperBound->value());
  blackWhiteOptions.setWolfLowerBound(lowerBound->value());
  BlackWhiteOptions::ColorSegmenterOptions segmenterOptions = blackWhiteOptions.getColorSegmenterOptions();
  segmenterOptions.setEnabled(colorSegmentationCB->isChecked());
  segmenterOptions.setNoiseReduction(reduceNoiseSB->value());
  segmenterOptions.setRedThresholdAdjustment(redAdjustmentSB->value());
  segmenterOptions.setGreenThresholdAdjustment(greenAdjustmentSB->value());
  segmenterOptions.setBlueThresholdAdjustment(blueAdjustmentSB->value());
  blackWhiteOptions.setColorSegmenterOptions(segmenterOptions);
  colorParams.setBlackWhiteOptions(blackWhiteOptions);

  SplittingOptions splittingOptions;
  splittingOptions.setSplitOutput(splittingCB->isChecked());
  splittingOptions.setSplittingMode(bwForegroundRB->isChecked() ? BLACK_AND_WHITE_FOREGROUND : COLOR_FOREGROUND);
  splittingOptions.setOriginalBackground(originalBackgroundCB->isChecked());

  PictureShapeOptions pictureShapeOptions;
  pictureShapeOptions.setPictureShape(static_cast<PictureShape>(pictureShapeSelector->currentData().toInt()));
  pictureShapeOptions.setSensitivity(pictureShapeSensitivitySB->value());
  pictureShapeOptions.setHigherSearchSensitivity(higherSearchSensitivityCB->isChecked());

  DewarpingOptions dewarpingOptions;
  dewarpingOptions.setDewarpingMode(static_cast<DewarpingMode>(dewarpingModeCB->currentData().toInt()));
  dewarpingOptions.setPostDeskew(dewarpingPostDeskewCB->isChecked());

  double despeckleLevel;
  if (despeckleCB->isChecked()) {
    despeckleLevel = 0.1 * despeckleSlider->value();
  } else {
    despeckleLevel = 0;
  }

  DefaultParams::OutputParams outputParams(Dpi(dpi, dpi), colorParams, splittingOptions, pictureShapeOptions,
                                           DepthPerception(0.1 * depthPerceptionSlider->value()), dewarpingOptions,
                                           despeckleLevel);

  std::unique_ptr<DefaultParams> defaultParams = std::make_unique<DefaultParams>(
      fixOrientationParams, deskewParams, pageSplitParams, selectContentParams, pageLayoutParams, outputParams);
  defaultParams->setUnits(currentUnits);

  return defaultParams;
}

void DefaultParamsDialog::updateUnits(const Units units) {
  currentUnits = units;
  unitsLabel->setText(unitsToLocalizedString(units));

  {
    int decimals;
    double step;
    switch (units) {
      case PIXELS:
      case MILLIMETRES:
        decimals = 1;
        step = 1.0;
        break;
      default:
        decimals = 2;
        step = 0.01;
        break;
    }

    widthSpinBox->setDecimals(decimals);
    widthSpinBox->setSingleStep(step);
    heightSpinBox->setDecimals(decimals);
    heightSpinBox->setSingleStep(step);
  }

  {
    int decimals;
    double step;
    switch (units) {
      case PIXELS:
      case MILLIMETRES:
        decimals = 1;
        step = 1.0;
        break;
      default:
        decimals = 2;
        step = 0.01;
        break;
    }

    topMarginSpinBox->setDecimals(decimals);
    topMarginSpinBox->setSingleStep(step);
    bottomMarginSpinBox->setDecimals(decimals);
    bottomMarginSpinBox->setSingleStep(step);
    leftMarginSpinBox->setDecimals(decimals);
    leftMarginSpinBox->setSingleStep(step);
    rightMarginSpinBox->setDecimals(decimals);
    rightMarginSpinBox->setSingleStep(step);
  }
}

void DefaultParamsDialog::setLinkButtonLinked(QToolButton* button, bool linked) {
  button->setIcon(linked ? chainIcon : brokenChainIcon);
}

void DefaultParamsDialog::topBottomLinkClicked() {
  topBottomLinkEnabled = !topBottomLinkEnabled;
  setLinkButtonLinked(topBottomLink, topBottomLinkEnabled);
  if (topBottomLinkEnabled && (topMarginSpinBox->value() != bottomMarginSpinBox->value())) {
    const double new_margin = std::min(topMarginSpinBox->value(), bottomMarginSpinBox->value());
    topMarginSpinBox->setValue(new_margin);
    bottomMarginSpinBox->setValue(new_margin);
  }
}

void DefaultParamsDialog::leftRightLinkClicked() {
  leftRightLinkEnabled = !leftRightLinkEnabled;
  setLinkButtonLinked(leftRightLink, leftRightLinkEnabled);
  if (leftRightLinkEnabled && (leftMarginSpinBox->value() != rightMarginSpinBox->value())) {
    const double new_margin = std::min(leftMarginSpinBox->value(), rightMarginSpinBox->value());
    leftMarginSpinBox->setValue(new_margin);
    rightMarginSpinBox->setValue(new_margin);
  }
}

void DefaultParamsDialog::horMarginsChanged(double val) {
  if (ignoreMarginChanges) {
    return;
  }
  if (leftRightLinkEnabled) {
    const ScopedIncDec<int> scopeGuard(ignoreMarginChanges);
    leftMarginSpinBox->setValue(val);
    rightMarginSpinBox->setValue(val);
  }
}

void DefaultParamsDialog::vertMarginsChanged(double val) {
  if (ignoreMarginChanges) {
    return;
  }
  if (topBottomLinkEnabled) {
    const ScopedIncDec<int> scopeGuard(ignoreMarginChanges);
    topMarginSpinBox->setValue(val);
    bottomMarginSpinBox->setValue(val);
  }
}

void DefaultParamsDialog::thresholdSliderValueChanged(int value) {
  const QString tooltip_text(QString::number(value));
  thresholdSlider->setToolTip(tooltip_text);

  thresholLabel->setText(QString::number(value));

  // Show the tooltip immediately.
  const QPoint center(thresholdSlider->rect().center());
  QPoint tooltip_pos(thresholdSlider->mapFromGlobal(QCursor::pos()));
  tooltip_pos.setY(center.y());
  tooltip_pos.setX(qBound(0, tooltip_pos.x(), thresholdSlider->width()));
  tooltip_pos = thresholdSlider->mapToGlobal(tooltip_pos);
  QToolTip::showText(tooltip_pos, tooltip_text, thresholdSlider);
}

void DefaultParamsDialog::setLighterThreshold() {
  thresholdSlider->setValue(thresholdSlider->value() - 1);
  thresholLabel->setText(QString::number(thresholdSlider->value()));
}

void DefaultParamsDialog::setDarkerThreshold() {
  thresholdSlider->setValue(thresholdSlider->value() + 1);
  thresholLabel->setText(QString::number(thresholdSlider->value()));
}

void DefaultParamsDialog::setNeutralThreshold() {
  thresholdSlider->setValue(0);
  thresholLabel->setText(QString::number(thresholdSlider->value()));
}

void DefaultParamsDialog::dpiSelectionChanged(int index) {
  dpiSelector->setEditable(index == customDpiItemIdx);
  if (index == customDpiItemIdx) {
    dpiSelector->setEditText(customDpiValue);
    dpiSelector->lineEdit()->selectAll();
    // It looks like we need to set a new validator
    // every time we make the combo box editable.
    dpiSelector->setValidator(new QIntValidator(0, 9999, dpiSelector));
  }
}

void DefaultParamsDialog::dpiEditTextChanged(const QString& text) {
  if (dpiSelector->currentIndex() == customDpiItemIdx) {
    customDpiValue = text;
  }
}

void DefaultParamsDialog::depthPerceptionChangedSlot(const int val) {
  const QString tooltip_text(QString::number(0.1 * val));
  depthPerceptionSlider->setToolTip(tooltip_text);

  // Show the tooltip immediately.
  const QPoint center(depthPerceptionSlider->rect().center());
  QPoint tooltip_pos(depthPerceptionSlider->mapFromGlobal(QCursor::pos()));
  tooltip_pos.setY(center.y());
  tooltip_pos.setX(qBound(0, tooltip_pos.x(), depthPerceptionSlider->width()));
  tooltip_pos = depthPerceptionSlider->mapToGlobal(tooltip_pos);
  QToolTip::showText(tooltip_pos, tooltip_text, depthPerceptionSlider);
}

void DefaultParamsDialog::profileChanged(const int index) {
  if (ignoreProfileChanges) {
    return;
  }

  profileCB->setEditable(index == customProfileItemIdx);
  if (index == customProfileItemIdx) {
    profileCB->setEditText(profileCB->currentText());
    profileCB->lineEdit()->selectAll();
    profileCB->setFocus();
    profileSaveButton->setEnabled(true);
    profileDeleteButton->setEnabled(false);
    setTabWidgetsEnabled(true);

    if (DefaultParamsProvider::getInstance()->getProfileName() == "Custom") {
      loadParams(DefaultParamsProvider::getInstance()->getParams());
    } else {
      updateUnits(UnitsProvider::getInstance()->getUnits());
    }
  } else if (profileCB->currentData().toString() == "Default") {
    profileSaveButton->setEnabled(false);
    profileDeleteButton->setEnabled(false);
    setTabWidgetsEnabled(false);

    std::unique_ptr<DefaultParams> defaultProfile = profileManager.createDefaultProfile();
    loadParams(*defaultProfile);
  } else if (profileCB->currentData().toString() == "Source") {
    profileSaveButton->setEnabled(false);
    profileDeleteButton->setEnabled(false);
    setTabWidgetsEnabled(false);

    std::unique_ptr<DefaultParams> sourceProfile = profileManager.createSourceProfile();
    loadParams(*sourceProfile);
  } else {
    std::unique_ptr<DefaultParams> profile = profileManager.readProfile(profileCB->itemData(index).toString());
    if (profile != nullptr) {
      profileSaveButton->setEnabled(true);
      profileDeleteButton->setEnabled(true);
      setTabWidgetsEnabled(true);

      loadParams(*profile);
    } else {
      QMessageBox::critical(this, tr("Error"), tr("Error loading the profile."));
      profileCB->setCurrentIndex(0);
      profileCB->removeItem(index);
      customProfileItemIdx--;

      profileSaveButton->setEnabled(false);
      profileDeleteButton->setEnabled(false);
      setTabWidgetsEnabled(false);

      std::unique_ptr<DefaultParams> defaultProfile = profileManager.createDefaultProfile();
      loadParams(*defaultProfile);
    }
  }
}

void DefaultParamsDialog::profileSavePressed() {
  const ScopedIncDec<int> scopeGuard(ignoreProfileChanges);

  if (isProfileNameReserved(profileCB->currentText())) {
    QMessageBox::information(this, tr("Error"),
                             tr("The name conflicts with a default profile name. Please enter a different name."));
    return;
  }

  if (profileManager.writeProfile(*buildParams(), profileCB->currentText())) {
    if (profileCB->currentIndex() == customProfileItemIdx) {
      const QString profileName = profileCB->currentText();
      profileCB->setItemData(profileCB->currentIndex(), profileName);
      profileCB->setItemText(profileCB->currentIndex(), profileName);
      customProfileItemIdx = profileCB->count();
      profileCB->addItem(tr("Custom"), "Custom");

      profileCB->setEditable(false);
      profileDeleteButton->setEnabled(true);
    }
  } else {
    QMessageBox::critical(this, tr("Error"), tr("Error saving the profile."));
  }
}

void DefaultParamsDialog::profileDeletePressed() {
  if (profileManager.deleteProfile(profileCB->currentText())) {
    {
      const ScopedIncDec<int> scopeGuard(ignoreProfileChanges);

      const int deletedProfileIndex = profileCB->currentIndex();
      profileCB->setCurrentIndex(customProfileItemIdx--);
      profileCB->removeItem(deletedProfileIndex);
    }
    profileChanged(customProfileItemIdx);
  } else {
    QMessageBox::critical(this, tr("Error"), tr("Error deleting the profile."));
  }
}

bool DefaultParamsDialog::isProfileNameReserved(const QString& name) {
  return reservedProfileNames.find(name) != reservedProfileNames.end();
}

void DefaultParamsDialog::commitChanges() {
  const QString profile = profileCB->currentData().toString();
  std::unique_ptr<DefaultParams> params;
  if (profile == "Default") {
    params = profileManager.createDefaultProfile();
  } else if (profile == "Source") {
    params = profileManager.createSourceProfile();
  } else {
    params = buildParams();
  }
  DefaultParamsProvider::getInstance()->setParams(std::move(params), profile);
  QSettings().setValue("settings/current_profile", profile);
}

void DefaultParamsDialog::setTabWidgetsEnabled(const bool enabled) {
  for (int i = 0; i < tabWidget->count(); i++) {
    QWidget* widget = tabWidget->widget(i);
    widget->setEnabled(enabled);
  }
}

void DefaultParamsDialog::colorSegmentationToggled(bool checked) {
  segmenterOptionsWidget->setEnabled(checked);
  if ((colorModeSelector->currentData() == BLACK_AND_WHITE) || (colorModeSelector->currentData() == MIXED)) {
    posterizeCB->setEnabled(checked);
    posterizeOptionsWidget->setEnabled(checked && posterizeCB->isChecked());
  }
}

void DefaultParamsDialog::posterizeToggled(bool checked) {
  posterizeOptionsWidget->setEnabled(checked);
}

void DefaultParamsDialog::updateAlignmentButtonsEnabled() {
  bool enableHorizontalButtons;
  bool enableVerticalButtons;
  if ((alignmentMode->currentIndex() == 0) || (alignmentMode->currentIndex() == 2)) {
    enableHorizontalButtons = !autoHorizontalAligningCB->isChecked() ? alignWithOthersCB->isChecked() : false;
    enableVerticalButtons = !autoVerticalAligningCB->isChecked() ? alignWithOthersCB->isChecked() : false;
  } else {
    enableHorizontalButtons = enableVerticalButtons = alignWithOthersCB->isChecked();
  }

  alignTopLeftBtn->setEnabled(enableHorizontalButtons && enableVerticalButtons);
  alignTopBtn->setEnabled(enableVerticalButtons);
  alignTopRightBtn->setEnabled(enableHorizontalButtons && enableVerticalButtons);
  alignLeftBtn->setEnabled(enableHorizontalButtons);
  alignCenterBtn->setEnabled(enableHorizontalButtons || enableVerticalButtons);
  alignRightBtn->setEnabled(enableHorizontalButtons);
  alignBottomLeftBtn->setEnabled(enableHorizontalButtons && enableVerticalButtons);
  alignBottomBtn->setEnabled(enableVerticalButtons);
  alignBottomRightBtn->setEnabled(enableHorizontalButtons && enableVerticalButtons);
}

void DefaultParamsDialog::updateAutoModeButtons() {
  if ((alignmentMode->currentIndex() == 0) || (alignmentMode->currentIndex() == 2)) {
    if (autoVerticalAligningCB->isChecked() && !autoHorizontalAligningCB->isChecked()) {
      autoVerticalAligningCB->setEnabled(false);
    } else if (autoHorizontalAligningCB->isChecked() && !autoVerticalAligningCB->isChecked()) {
      autoHorizontalAligningCB->setEnabled(false);
    } else {
      autoVerticalAligningCB->setEnabled(true);
      autoHorizontalAligningCB->setEnabled(true);
    }
  }

  if (autoVerticalAligningCB->isChecked() && !autoHorizontalAligningCB->isChecked()) {
    switch (alignmentByButton.at(getCheckedAlignmentButton()).horizontal()) {
      case Alignment::LEFT:
        alignLeftBtn->setChecked(true);
        break;
      case Alignment::RIGHT:
        alignRightBtn->setChecked(true);
        break;
      default:
        alignCenterBtn->setChecked(true);
        break;
    }
  } else if (autoHorizontalAligningCB->isChecked() && !autoVerticalAligningCB->isChecked()) {
    switch (alignmentByButton.at(getCheckedAlignmentButton()).vertical()) {
      case Alignment::TOP:
        alignTopBtn->setChecked(true);
        break;
      case Alignment::BOTTOM:
        alignBottomBtn->setChecked(true);
        break;
      default:
        alignCenterBtn->setChecked(true);
        break;
    }
  }
}

QToolButton* DefaultParamsDialog::getCheckedAlignmentButton() const {
  auto* checkedButton = dynamic_cast<QToolButton*>(alignmentButtonGroup->checkedButton());
  if (!checkedButton) {
    checkedButton = alignCenterBtn;
  }

  return checkedButton;
}

void DefaultParamsDialog::autoHorizontalAligningToggled(bool) {
  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();
}

void DefaultParamsDialog::autoVerticalAligningToggled(bool) {
  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();
}

void DefaultParamsDialog::despeckleToggled(bool checked) {
  despeckleSlider->setEnabled(checked);
}

void DefaultParamsDialog::despeckleSliderValueChanged(int value) {
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
}
