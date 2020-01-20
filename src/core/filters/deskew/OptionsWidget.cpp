// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"

#include <utility>

#include "ApplyDialog.h"
#include "Settings.h"

namespace deskew {
const double OptionsWidget::MAX_ANGLE = 45.0;

OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::move(settings)),
      m_pageSelectionAccessor(pageSelectionAccessor),
      m_connectionManager(std::bind(&OptionsWidget::setupUiConnections, this)) {
  setupUi(this);
  angleSpinBox->setSuffix(QChar(0x00B0));  // the degree symbol
  angleSpinBox->setRange(-MAX_ANGLE, MAX_ANGLE);
  angleSpinBox->adjustSize();
  setSpinBoxUnknownState();

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::showDeskewDialog() {
  auto* dialog = new ApplyDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Deskew"));
  connect(dialog, SIGNAL(appliedTo(const std::set<PageId>&)), this, SLOT(appliedTo(const std::set<PageId>&)));
  connect(dialog, SIGNAL(appliedToAllPages(const std::set<PageId>&)), this,
          SLOT(appliedToAllPages(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::appliedTo(const std::set<PageId>& pages) {
  if (pages.empty()) {
    return;
  }

  const Params params(m_uiData.effectiveDeskewAngle(), m_uiData.dependencies(), m_uiData.mode());
  m_settings->setDegrees(pages, params);

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& pageId : pages) {
      emit invalidateThumbnail(pageId);
    }
  }
}

void OptionsWidget::appliedToAllPages(const std::set<PageId>& pages) {
  if (pages.empty()) {
    return;
  }

  const Params params(m_uiData.effectiveDeskewAngle(), m_uiData.dependencies(), m_uiData.mode());
  m_settings->setDegrees(pages, params);
  emit invalidateAllThumbnails();
}

void OptionsWidget::manualDeskewAngleSetExternally(const double degrees) {
  m_uiData.setEffectiveDeskewAngle(degrees);
  m_uiData.setMode(MODE_MANUAL);
  updateModeIndication(MODE_MANUAL);
  setSpinBoxKnownState(degreesToSpinBox(degrees));
  commitCurrentParams();

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::preUpdateUI(const PageId& pageId) {
  auto block = m_connectionManager.getScopedBlock();

  m_pageId = pageId;
  setSpinBoxUnknownState();
  autoBtn->setChecked(true);
  autoBtn->setEnabled(false);
  manualBtn->setEnabled(false);
}

void OptionsWidget::postUpdateUI(const UiData& uiData) {
  auto block = m_connectionManager.getScopedBlock();

  m_uiData = uiData;
  autoBtn->setEnabled(true);
  manualBtn->setEnabled(true);
  updateModeIndication(uiData.mode());
  setSpinBoxKnownState(degreesToSpinBox(uiData.effectiveDeskewAngle()));
}

void OptionsWidget::spinBoxValueChanged(const double value) {
  if (m_ignoreSpinBoxChanges) {
    return;
  }

  const double degrees = spinBoxToDegrees(value);
  m_uiData.setEffectiveDeskewAngle(degrees);
  m_uiData.setMode(MODE_MANUAL);
  updateModeIndication(MODE_MANUAL);
  commitCurrentParams();

  emit manualDeskewAngleSet(degrees);
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::modeChanged(const bool autoMode) {
  if (autoMode) {
    m_uiData.setMode(MODE_AUTO);
    m_settings->clearPageParams(m_pageId);
    emit reloadRequested();
  } else {
    m_uiData.setMode(MODE_MANUAL);
    commitCurrentParams();
  }
}

void OptionsWidget::updateModeIndication(const AutoManualMode mode) {
  auto block = m_connectionManager.getScopedBlock();

  if (mode == MODE_AUTO) {
    autoBtn->setChecked(true);
  } else {
    manualBtn->setChecked(true);
  }
}

void OptionsWidget::setSpinBoxUnknownState() {
  auto block = m_connectionManager.getScopedBlock();

  angleSpinBox->setSpecialValueText("?");
  angleSpinBox->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  angleSpinBox->setValue(angleSpinBox->minimum());
  angleSpinBox->setEnabled(false);
}

void OptionsWidget::setSpinBoxKnownState(const double angle) {
  auto block = m_connectionManager.getScopedBlock();

  angleSpinBox->setSpecialValueText("");
  angleSpinBox->setValue(angle);

  // Right alignment doesn't work correctly, so we use the left one.
  angleSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  angleSpinBox->setEnabled(true);
}

void OptionsWidget::commitCurrentParams() {
  Params params(m_uiData.effectiveDeskewAngle(), m_uiData.dependencies(), m_uiData.mode());
  m_settings->setPageParams(m_pageId, params);
}

double OptionsWidget::spinBoxToDegrees(const double sbValue) {
  // The spin box shows the angle in a usual geometric way,
  // with positive angles going counter-clockwise.
  // Internally, we operate with angles going clockwise,
  // because the Y axis points downwards in computer graphics.
  return -sbValue;
}

double OptionsWidget::degreesToSpinBox(const double degrees) {
  // See above.
  return -degrees;
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void OptionsWidget::setupUiConnections() {
  CONNECT(angleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxValueChanged(double)));
  CONNECT(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
  CONNECT(applyDeskewBtn, SIGNAL(clicked()), this, SLOT(showDeskewDialog()));
}

#undef CONNECT

/*========================== OptionsWidget::UiData =========================*/

OptionsWidget::UiData::UiData() : m_effDeskewAngle(0.0), m_mode(MODE_AUTO) {}

OptionsWidget::UiData::~UiData() = default;

void OptionsWidget::UiData::setEffectiveDeskewAngle(const double degrees) {
  m_effDeskewAngle = degrees;
}

double OptionsWidget::UiData::effectiveDeskewAngle() const {
  return m_effDeskewAngle;
}

void OptionsWidget::UiData::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

const Dependencies& OptionsWidget::UiData::dependencies() const {
  return m_deps;
}

void OptionsWidget::UiData::setMode(const AutoManualMode mode) {
  m_mode = mode;
}

AutoManualMode OptionsWidget::UiData::mode() const {
  return m_mode;
}
}  // namespace deskew