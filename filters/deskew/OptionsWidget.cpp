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

#include <utility>
#include "ApplyDialog.h"
#include "ScopedIncDec.h"
#include "Settings.h"

namespace deskew {
const double OptionsWidget::MAX_ANGLE = 45.0;

OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor)
    : m_ptrSettings(std::move(settings)),
      m_ignoreAutoManualToggle(0),
      m_ignoreSpinBoxChanges(0),
      m_pageSelectionAccessor(page_selection_accessor) {
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
  m_ptrSettings->setDegrees(pages, params);

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }
}

void OptionsWidget::appliedToAllPages(const std::set<PageId>& pages) {
  if (pages.empty()) {
    return;
  }

  const Params params(m_uiData.effectiveDeskewAngle(), m_uiData.dependencies(), m_uiData.mode());
  m_ptrSettings->setDegrees(pages, params);
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

void OptionsWidget::preUpdateUI(const PageId& page_id) {
  removeUiConnections();

  ScopedIncDec<int> guard(m_ignoreAutoManualToggle);

  m_pageId = page_id;
  setSpinBoxUnknownState();
  autoBtn->setChecked(true);
  autoBtn->setEnabled(false);
  manualBtn->setEnabled(false);

  setupUiConnections();
}

void OptionsWidget::postUpdateUI(const UiData& ui_data) {
  removeUiConnections();

  m_uiData = ui_data;
  autoBtn->setEnabled(true);
  manualBtn->setEnabled(true);
  updateModeIndication(ui_data.mode());
  setSpinBoxKnownState(degreesToSpinBox(ui_data.effectiveDeskewAngle()));

  setupUiConnections();
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

void OptionsWidget::modeChanged(const bool auto_mode) {
  if (m_ignoreAutoManualToggle) {
    return;
  }

  if (auto_mode) {
    m_uiData.setMode(MODE_AUTO);
    m_ptrSettings->clearPageParams(m_pageId);
    emit reloadRequested();
  } else {
    m_uiData.setMode(MODE_MANUAL);
    commitCurrentParams();
  }
}

void OptionsWidget::updateModeIndication(const AutoManualMode mode) {
  ScopedIncDec<int> guard(m_ignoreAutoManualToggle);

  if (mode == MODE_AUTO) {
    autoBtn->setChecked(true);
  } else {
    manualBtn->setChecked(true);
  }
}

void OptionsWidget::setSpinBoxUnknownState() {
  ScopedIncDec<int> guard(m_ignoreSpinBoxChanges);

  angleSpinBox->setSpecialValueText("?");
  angleSpinBox->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  angleSpinBox->setValue(angleSpinBox->minimum());
  angleSpinBox->setEnabled(false);
}

void OptionsWidget::setSpinBoxKnownState(const double angle) {
  ScopedIncDec<int> guard(m_ignoreSpinBoxChanges);

  angleSpinBox->setSpecialValueText("");
  angleSpinBox->setValue(angle);

  // Right alignment doesn't work correctly, so we use the left one.
  angleSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  angleSpinBox->setEnabled(true);
}

void OptionsWidget::commitCurrentParams() {
  Params params(m_uiData.effectiveDeskewAngle(), m_uiData.dependencies(), m_uiData.mode());
  m_ptrSettings->setPageParams(m_pageId, params);
}

double OptionsWidget::spinBoxToDegrees(const double sb_value) {
  // The spin box shows the angle in a usual geometric way,
  // with positive angles going counter-clockwise.
  // Internally, we operate with angles going clockwise,
  // because the Y axis points downwards in computer graphics.
  return -sb_value;
}

double OptionsWidget::degreesToSpinBox(const double degrees) {
  // See above.
  return -degrees;
}

void OptionsWidget::setupUiConnections() {
  connect(angleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxValueChanged(double)));
  connect(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
  connect(applyDeskewBtn, SIGNAL(clicked()), this, SLOT(showDeskewDialog()));
}

void OptionsWidget::removeUiConnections() {
  disconnect(angleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxValueChanged(double)));
  disconnect(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
  disconnect(applyDeskewBtn, SIGNAL(clicked()), this, SLOT(showDeskewDialog()));
}

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