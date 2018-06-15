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
#include <UnitsProvider.h>
#include <QSettings>
#include <utility>
#include "../../Utils.h"
#include "ApplyDialog.h"
#include "ScopedIncDec.h"
#include "Settings.h"
#include "imageproc/Constants.h"

using namespace imageproc::constants;

namespace page_layout {
OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor)
    : m_ptrSettings(std::move(settings)),
      m_pageSelectionAccessor(page_selection_accessor),
      m_leftRightLinked(true),
      m_topBottomLinked(true) {
  {
    QSettings app_settings;
    m_leftRightLinked = app_settings.value("margins/leftRightLinked", true).toBool();
    m_topBottomLinked = app_settings.value("margins/topBottomLinked", true).toBool();
  }

  m_chainIcon.addPixmap(QPixmap(QString::fromLatin1(":/icons/stock-vchain-24.png")));
  m_brokenChainIcon.addPixmap(QPixmap(QString::fromLatin1(":/icons/stock-vchain-broken-24.png")));

  setupUi(this);
  updateLinkDisplay(topBottomLink, m_topBottomLinked);
  updateLinkDisplay(leftRightLink, m_leftRightLinked);
  updateAlignmentButtonsEnabled();

  Utils::mapSetValue(m_alignmentByButton, alignTopLeftBtn, Alignment(Alignment::TOP, Alignment::LEFT));
  Utils::mapSetValue(m_alignmentByButton, alignTopBtn, Alignment(Alignment::TOP, Alignment::HCENTER));
  Utils::mapSetValue(m_alignmentByButton, alignTopRightBtn, Alignment(Alignment::TOP, Alignment::RIGHT));
  Utils::mapSetValue(m_alignmentByButton, alignLeftBtn, Alignment(Alignment::VCENTER, Alignment::LEFT));
  Utils::mapSetValue(m_alignmentByButton, alignCenterBtn, Alignment(Alignment::VCENTER, Alignment::HCENTER));
  Utils::mapSetValue(m_alignmentByButton, alignRightBtn, Alignment(Alignment::VCENTER, Alignment::RIGHT));
  Utils::mapSetValue(m_alignmentByButton, alignBottomLeftBtn, Alignment(Alignment::BOTTOM, Alignment::LEFT));
  Utils::mapSetValue(m_alignmentByButton, alignBottomBtn, Alignment(Alignment::BOTTOM, Alignment::HCENTER));
  Utils::mapSetValue(m_alignmentByButton, alignBottomRightBtn, Alignment(Alignment::BOTTOM, Alignment::RIGHT));

  m_alignmentButtonGroup = std::make_unique<QButtonGroup>(this);
  for (const auto& buttonAndAlignment : m_alignmentByButton) {
    m_alignmentButtonGroup->addButton(buttonAndAlignment.first);
  }

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageInfo& page_info, const Margins& margins_mm, const Alignment& alignment) {
  removeUiConnections();

  m_pageId = page_info.id();
  m_dpi = page_info.metadata().dpi();
  m_marginsMM = margins_mm;
  m_alignment = alignment;

  auto old_ignore = static_cast<bool>(m_ignoreMarginChanges);
  m_ignoreMarginChanges = true;

  for (const auto& kv : m_alignmentByButton) {
    if (m_alignment.isAuto() || m_alignment.isOriginal()) {
      if (!m_alignment.isAutoHorizontal() && (kv.second.vertical() == Alignment::VCENTER)
          && (kv.second.horizontal() == m_alignment.horizontal())) {
        kv.first->setChecked(true);
        break;
      } else if (!m_alignment.isAutoVertical() && (kv.second.horizontal() == Alignment::HCENTER)
                 && (kv.second.vertical() == m_alignment.vertical())) {
        kv.first->setChecked(true);
        break;
      }
    } else if (kv.second == m_alignment) {
      kv.first->setChecked(true);
      break;
    }
  }

  updateUnits(UnitsProvider::getInstance()->getUnits());

  alignWithOthersCB->blockSignals(true);
  alignWithOthersCB->setChecked(!alignment.isNull());
  alignWithOthersCB->blockSignals(false);

  alignmentMode->blockSignals(true);
  if (alignment.isAuto()) {
    alignmentMode->setCurrentIndex(0);
    autoAlignSettingsGroup->setVisible(true);
  } else if (alignment.isOriginal()) {
    alignmentMode->setCurrentIndex(2);
    autoAlignSettingsGroup->setVisible(true);
  } else {
    alignmentMode->setCurrentIndex(1);
    autoAlignSettingsGroup->setVisible(false);
  }
  alignmentMode->blockSignals(false);
  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();

  autoMargins->setChecked(m_ptrSettings->isPageAutoMarginsEnabled(m_pageId));
  updateMarginsControlsEnabled();

  m_leftRightLinked = m_leftRightLinked && (margins_mm.left() == margins_mm.right());
  m_topBottomLinked = m_topBottomLinked && (margins_mm.top() == margins_mm.bottom());
  updateLinkDisplay(topBottomLink, m_topBottomLinked);
  updateLinkDisplay(leftRightLink, m_leftRightLinked);

  marginsGroup->setEnabled(false);
  alignmentGroup->setEnabled(false);

  m_ignoreMarginChanges = old_ignore;

  setupUiConnections();
}  // OptionsWidget::preUpdateUI

void OptionsWidget::postUpdateUI() {
  removeUiConnections();

  marginsGroup->setEnabled(true);
  alignmentGroup->setEnabled(true);

  m_marginsMM = m_ptrSettings->getHardMarginsMM(m_pageId);
  updateMarginsDisplay();

  setupUiConnections();
}

void OptionsWidget::marginsSetExternally(const Margins& margins_mm) {
  m_marginsMM = margins_mm;

  if (autoMargins->isChecked()) {
    autoMargins->setChecked(false);
    m_ptrSettings->setPageAutoMarginsEnabled(m_pageId, false);
    updateMarginsControlsEnabled();
  }

  updateMarginsDisplay();
}

void OptionsWidget::updateUnits(const Units units) {
  removeUiConnections();

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

  updateMarginsDisplay();

  setupUiConnections();
}

void OptionsWidget::horMarginsChanged(const double val) {
  if (m_ignoreMarginChanges) {
    return;
  }

  if (m_leftRightLinked) {
    const ScopedIncDec<int> ignore_scope(m_ignoreMarginChanges);
    leftMarginSpinBox->setValue(val);
    rightMarginSpinBox->setValue(val);
  }

  double dummy;
  double leftMarginSpinBoxValue = leftMarginSpinBox->value();
  double rightMarginSpinBoxValue = rightMarginSpinBox->value();
  UnitsProvider::getInstance()->convertTo(leftMarginSpinBoxValue, dummy, MILLIMETRES, m_dpi);
  UnitsProvider::getInstance()->convertTo(rightMarginSpinBoxValue, dummy, MILLIMETRES, m_dpi);

  m_marginsMM.setLeft(leftMarginSpinBoxValue);
  m_marginsMM.setRight(rightMarginSpinBoxValue);

  emit marginsSetLocally(m_marginsMM);
}

void OptionsWidget::vertMarginsChanged(const double val) {
  if (m_ignoreMarginChanges) {
    return;
  }

  if (m_topBottomLinked) {
    const ScopedIncDec<int> ignore_scope(m_ignoreMarginChanges);
    topMarginSpinBox->setValue(val);
    bottomMarginSpinBox->setValue(val);
  }

  double dummy;
  double topMarginSpinBoxValue = topMarginSpinBox->value();
  double bottomMarginSpinBoxValue = bottomMarginSpinBox->value();
  UnitsProvider::getInstance()->convertTo(dummy, topMarginSpinBoxValue, MILLIMETRES, m_dpi);
  UnitsProvider::getInstance()->convertTo(dummy, bottomMarginSpinBoxValue, MILLIMETRES, m_dpi);

  m_marginsMM.setTop(topMarginSpinBoxValue);
  m_marginsMM.setBottom(bottomMarginSpinBoxValue);

  emit marginsSetLocally(m_marginsMM);
}

void OptionsWidget::topBottomLinkClicked() {
  m_topBottomLinked = !m_topBottomLinked;
  QSettings().setValue("margins/topBottomLinked", m_topBottomLinked);
  updateLinkDisplay(topBottomLink, m_topBottomLinked);
  topBottomLinkToggled(m_topBottomLinked);
}

void OptionsWidget::leftRightLinkClicked() {
  m_leftRightLinked = !m_leftRightLinked;
  QSettings().setValue("margins/leftRightLinked", m_leftRightLinked);
  updateLinkDisplay(leftRightLink, m_leftRightLinked);
  leftRightLinkToggled(m_leftRightLinked);
}

void OptionsWidget::alignWithOthersToggled() {
  m_alignment.setNull(!alignWithOthersCB->isChecked());
  updateAlignmentButtonsEnabled();
  emit alignmentChanged(m_alignment);
}

void OptionsWidget::autoMarginsToggled(bool checked) {
  if (m_ignoreMarginChanges) {
    return;
  }

  m_ptrSettings->setPageAutoMarginsEnabled(m_pageId, checked);
  updateMarginsControlsEnabled();

  emit reloadRequested();
}

void OptionsWidget::alignmentModeChanged(int idx) {
  switch (idx) {
    case 0:
      m_alignment.setVertical(Alignment::VAUTO);
      m_alignment.setHorizontal(Alignment::HAUTO);
      autoAlignSettingsGroup->setVisible(true);
      updateAutoModeButtons();
      break;
    case 1:
      m_alignment = m_alignmentByButton.at(getCheckedAlignmentButton());
      autoAlignSettingsGroup->setVisible(false);
      break;
    case 2:
      m_alignment.setVertical(Alignment::VORIGINAL);
      if (m_ptrSettings->isPageAutoMarginsEnabled(m_pageId)) {
        m_alignment.setHorizontal(Alignment::HORIGINAL);
      } else {
        m_alignment.setHorizontal(m_alignmentByButton.at(getCheckedAlignmentButton()).horizontal());
      }
      autoAlignSettingsGroup->setVisible(true);
      updateAutoModeButtons();
      break;
    default:
      break;
  }

  updateAlignmentButtonsEnabled();
  emit alignmentChanged(m_alignment);
}

void OptionsWidget::alignmentButtonClicked() {
  if (m_ignoreAlignmentButtonsChanges) {
    return;
  }

  auto* const button = dynamic_cast<QToolButton*>(sender());
  assert(button);

  const Alignment& alignment = m_alignmentByButton.at(button);

  if (m_alignment.isAutoVertical()) {
    m_alignment.setHorizontal(alignment.horizontal());
  } else if (m_alignment.isAutoHorizontal()) {
    m_alignment.setVertical(alignment.vertical());
  } else {
    m_alignment = alignment;
  }

  emit alignmentChanged(m_alignment);
}

void OptionsWidget::showApplyMarginsDialog() {
  auto* dialog = new ApplyDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Margins"));
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&)), this, SLOT(applyMargins(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::showApplyAlignmentDialog() {
  auto* dialog = new ApplyDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Apply Alignment"));
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&)), this, SLOT(applyAlignment(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::applyMargins(const std::set<PageId>& pages) {
  if (pages.empty()) {
    return;
  }

  const bool autoMarginsEnabled = m_ptrSettings->isPageAutoMarginsEnabled(m_pageId);
  for (const PageId& page_id : pages) {
    if (page_id == m_pageId) {
      continue;
    }

    m_ptrSettings->setPageAutoMarginsEnabled(page_id, autoMarginsEnabled);
    if (autoMarginsEnabled) {
      m_ptrSettings->invalidateContentSize(page_id);
    } else {
      m_ptrSettings->setHardMarginsMM(page_id, m_marginsMM);
    }
  }

  emit aggregateHardSizeChanged();
  emit invalidateAllThumbnails();
}

void OptionsWidget::applyAlignment(const std::set<PageId>& pages) {
  if (pages.empty()) {
    return;
  }

  for (const PageId& page_id : pages) {
    if (page_id == m_pageId) {
      continue;
    }

    m_ptrSettings->setPageAlignment(page_id, m_alignment);
  }

  emit invalidateAllThumbnails();
}

void OptionsWidget::updateMarginsDisplay() {
  const ScopedIncDec<int> ignore_scope(m_ignoreMarginChanges);

  double topMarginValue = m_marginsMM.top();
  double bottomMarginValue = m_marginsMM.bottom();
  double leftMarginValue = m_marginsMM.left();
  double rightMarginValue = m_marginsMM.right();
  UnitsProvider::getInstance()->convertFrom(leftMarginValue, topMarginValue, MILLIMETRES, m_dpi);
  UnitsProvider::getInstance()->convertFrom(rightMarginValue, bottomMarginValue, MILLIMETRES, m_dpi);

  topMarginSpinBox->setValue(topMarginValue);
  bottomMarginSpinBox->setValue(bottomMarginValue);
  leftMarginSpinBox->setValue(leftMarginValue);
  rightMarginSpinBox->setValue(rightMarginValue);

  m_leftRightLinked = m_leftRightLinked && (leftMarginSpinBox->value() == rightMarginSpinBox->value());
  m_topBottomLinked = m_topBottomLinked && (topMarginSpinBox->value() == bottomMarginSpinBox->value());
  updateLinkDisplay(topBottomLink, m_topBottomLinked);
  updateLinkDisplay(leftRightLink, m_leftRightLinked);
}

void OptionsWidget::updateLinkDisplay(QToolButton* button, const bool linked) {
  button->setIcon(linked ? m_chainIcon : m_brokenChainIcon);
}

void OptionsWidget::updateAlignmentButtonsEnabled() {
  bool enableHorizontalButtons = !m_alignment.isAutoHorizontal() ? alignWithOthersCB->isChecked() : false;
  bool enableVerticalButtons = !m_alignment.isAutoVertical() ? alignWithOthersCB->isChecked() : false;

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

void OptionsWidget::updateMarginsControlsEnabled() {
  const bool enabled = !m_ptrSettings->isPageAutoMarginsEnabled(m_pageId);

  topMarginSpinBox->setEnabled(enabled);
  bottomMarginSpinBox->setEnabled(enabled);
  leftMarginSpinBox->setEnabled(enabled);
  rightMarginSpinBox->setEnabled(enabled);
  topBottomLink->setEnabled(enabled);
  leftRightLink->setEnabled(enabled);
}

void OptionsWidget::setupUiConnections() {
  connect(topMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  connect(bottomMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  connect(leftMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  connect(rightMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  connect(autoMargins, SIGNAL(toggled(bool)), this, SLOT(autoMarginsToggled(bool)));
  connect(alignmentMode, SIGNAL(currentIndexChanged(int)), this, SLOT(alignmentModeChanged(int)));
  connect(topBottomLink, SIGNAL(clicked()), this, SLOT(topBottomLinkClicked()));
  connect(leftRightLink, SIGNAL(clicked()), this, SLOT(leftRightLinkClicked()));
  connect(applyMarginsBtn, SIGNAL(clicked()), this, SLOT(showApplyMarginsDialog()));
  connect(alignWithOthersCB, SIGNAL(toggled(bool)), this, SLOT(alignWithOthersToggled()));
  connect(applyAlignmentBtn, SIGNAL(clicked()), this, SLOT(showApplyAlignmentDialog()));
  connect(autoHorizontalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoHorizontalAligningToggled(bool)));
  connect(autoVerticalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoVerticalAligningToggled(bool)));

  for (const auto& kv : m_alignmentByButton) {
    connect(kv.first, SIGNAL(clicked()), this, SLOT(alignmentButtonClicked()));
  }
}

void OptionsWidget::removeUiConnections() {
  disconnect(topMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  disconnect(bottomMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  disconnect(leftMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  disconnect(rightMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  disconnect(autoMargins, SIGNAL(toggled(bool)), this, SLOT(autoMarginsToggled(bool)));
  disconnect(alignmentMode, SIGNAL(currentIndexChanged(int)), this, SLOT(alignmentModeChanged(int)));
  disconnect(topBottomLink, SIGNAL(clicked()), this, SLOT(topBottomLinkClicked()));
  disconnect(leftRightLink, SIGNAL(clicked()), this, SLOT(leftRightLinkClicked()));
  disconnect(applyMarginsBtn, SIGNAL(clicked()), this, SLOT(showApplyMarginsDialog()));
  disconnect(alignWithOthersCB, SIGNAL(toggled(bool)), this, SLOT(alignWithOthersToggled()));
  disconnect(applyAlignmentBtn, SIGNAL(clicked()), this, SLOT(showApplyAlignmentDialog()));
  disconnect(autoHorizontalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoHorizontalAligningToggled(bool)));
  disconnect(autoVerticalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoVerticalAligningToggled(bool)));

  for (const auto& kv : m_alignmentByButton) {
    disconnect(kv.first, SIGNAL(clicked()), this, SLOT(alignmentButtonClicked()));
  }
}

bool OptionsWidget::leftRightLinked() const {
  return m_leftRightLinked;
}

bool OptionsWidget::topBottomLinked() const {
  return m_topBottomLinked;
}

const Margins& OptionsWidget::marginsMM() const {
  return m_marginsMM;
}

const Alignment& OptionsWidget::alignment() const {
  return m_alignment;
}

void OptionsWidget::autoHorizontalAligningToggled(const bool checked) {
  if (checked) {
    m_alignment.setHorizontal((alignmentMode->currentIndex() == 0) ? Alignment::HAUTO : Alignment::HORIGINAL);
  } else {
    m_alignment.setHorizontal(m_alignmentByButton.at(getCheckedAlignmentButton()).horizontal());
  }

  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();
  emit alignmentChanged(m_alignment);
}

void OptionsWidget::autoVerticalAligningToggled(const bool checked) {
  if (checked) {
    m_alignment.setVertical((alignmentMode->currentIndex() == 0) ? Alignment::VAUTO : Alignment::VORIGINAL);
  } else {
    m_alignment.setVertical(m_alignmentByButton.at(getCheckedAlignmentButton()).vertical());
  }

  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();
  emit alignmentChanged(m_alignment);
}

void OptionsWidget::updateAutoModeButtons() {
  const ScopedIncDec<int> scope_guard(m_ignoreAlignmentButtonsChanges);

  if (m_alignment.isAuto() || m_alignment.isOriginal()) {
    autoVerticalAligningCB->setChecked(m_alignment.isAutoVertical());
    autoHorizontalAligningCB->setChecked(m_alignment.isAutoHorizontal());

    if (autoVerticalAligningCB->isChecked() && !autoHorizontalAligningCB->isChecked()) {
      autoVerticalAligningCB->setEnabled(false);
    } else if (autoHorizontalAligningCB->isChecked() && !autoVerticalAligningCB->isChecked()) {
      autoHorizontalAligningCB->setEnabled(false);
    } else {
      autoVerticalAligningCB->setEnabled(true);
      autoHorizontalAligningCB->setEnabled(true);
    }
  }

  if (m_alignment.isAutoVertical() && !m_alignment.isAutoHorizontal()) {
    switch (m_alignmentByButton.at(getCheckedAlignmentButton()).horizontal()) {
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
  } else if (m_alignment.isAutoHorizontal() && !m_alignment.isAutoVertical()) {
    switch (m_alignmentByButton.at(getCheckedAlignmentButton()).vertical()) {
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

QToolButton* OptionsWidget::getCheckedAlignmentButton() const {
  auto* checkedButton = dynamic_cast<QToolButton*>(m_alignmentButtonGroup->checkedButton());
  if (!checkedButton) {
    checkedButton = alignCenterBtn;
  }

  return checkedButton;
}
}  // namespace page_layout
