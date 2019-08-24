// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"
#include <Constants.h>
#include <UnitsProvider.h>
#include <core/IconProvider.h>
#include <QSettings>
#include <utility>
#include "../../Utils.h"
#include "ApplyDialog.h"
#include "ScopedIncDec.h"
#include "Settings.h"

using namespace core;
using namespace constants;

namespace page_layout {
OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::move(settings)),
      m_pageSelectionAccessor(pageSelectionAccessor),
      m_leftRightLinked(true),
      m_topBottomLinked(true) {
  {
    QSettings appSettings;
    m_leftRightLinked = appSettings.value("margins/leftRightLinked", true).toBool();
    m_topBottomLinked = appSettings.value("margins/topBottomLinked", true).toBool();
  }

  setupUi(this);
  setupIcons();

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

void OptionsWidget::preUpdateUI(const PageInfo& pageInfo, const Margins& marginsMm, const Alignment& alignment) {
  removeUiConnections();

  m_pageId = pageInfo.id();
  m_dpi = pageInfo.metadata().dpi();
  m_marginsMM = marginsMm;
  m_alignment = alignment;

  auto oldIgnore = static_cast<bool>(m_ignoreMarginChanges);
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

  onUnitsChanged(UnitsProvider::getInstance().getUnits());

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
  updateAlignmentModeEnabled();
  updateAlignmentButtonsEnabled();
  updateAutoModeButtons();

  autoMargins->setChecked(m_settings->isPageAutoMarginsEnabled(m_pageId));
  updateMarginsControlsEnabled();

  m_leftRightLinked = m_leftRightLinked && (marginsMm.left() == marginsMm.right());
  m_topBottomLinked = m_topBottomLinked && (marginsMm.top() == marginsMm.bottom());
  updateLinkDisplay(topBottomLink, m_topBottomLinked);
  updateLinkDisplay(leftRightLink, m_leftRightLinked);

  marginsGroup->setEnabled(false);
  alignmentGroup->setEnabled(false);

  m_ignoreMarginChanges = oldIgnore;

  setupUiConnections();
}  // OptionsWidget::preUpdateUI

void OptionsWidget::postUpdateUI() {
  removeUiConnections();

  marginsGroup->setEnabled(true);
  alignmentGroup->setEnabled(true);

  m_marginsMM = m_settings->getHardMarginsMM(m_pageId);
  updateMarginsDisplay();

  setupUiConnections();
}

void OptionsWidget::marginsSetExternally(const Margins& marginsMm) {
  m_marginsMM = marginsMm;

  if (autoMargins->isChecked()) {
    autoMargins->setChecked(false);
    m_settings->setPageAutoMarginsEnabled(m_pageId, false);
    updateMarginsControlsEnabled();
  }

  updateMarginsDisplay();
}

void OptionsWidget::onUnitsChanged(Units units) {
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
    const ScopedIncDec<int> ignoreScope(m_ignoreMarginChanges);
    leftMarginSpinBox->setValue(val);
    rightMarginSpinBox->setValue(val);
  }

  double dummy;
  double leftMarginSpinBoxValue = leftMarginSpinBox->value();
  double rightMarginSpinBoxValue = rightMarginSpinBox->value();
  UnitsProvider::getInstance().convertTo(leftMarginSpinBoxValue, dummy, MILLIMETRES, m_dpi);
  UnitsProvider::getInstance().convertTo(rightMarginSpinBoxValue, dummy, MILLIMETRES, m_dpi);

  m_marginsMM.setLeft(leftMarginSpinBoxValue);
  m_marginsMM.setRight(rightMarginSpinBoxValue);

  emit marginsSetLocally(m_marginsMM);
}

void OptionsWidget::vertMarginsChanged(const double val) {
  if (m_ignoreMarginChanges) {
    return;
  }

  if (m_topBottomLinked) {
    const ScopedIncDec<int> ignoreScope(m_ignoreMarginChanges);
    topMarginSpinBox->setValue(val);
    bottomMarginSpinBox->setValue(val);
  }

  double dummy;
  double topMarginSpinBoxValue = topMarginSpinBox->value();
  double bottomMarginSpinBoxValue = bottomMarginSpinBox->value();
  UnitsProvider::getInstance().convertTo(dummy, topMarginSpinBoxValue, MILLIMETRES, m_dpi);
  UnitsProvider::getInstance().convertTo(dummy, bottomMarginSpinBoxValue, MILLIMETRES, m_dpi);

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
  if (m_ignoreAlignmentButtonsChanges) {
    return;
  }

  m_alignment.setNull(!alignWithOthersCB->isChecked());

  updateAlignmentModeEnabled();
  updateAlignmentButtonsEnabled();
  emit alignmentChanged(m_alignment);
}

void OptionsWidget::autoMarginsToggled(bool checked) {
  if (m_ignoreMarginChanges) {
    return;
  }

  m_settings->setPageAutoMarginsEnabled(m_pageId, checked);
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
      if (m_settings->isPageAutoMarginsEnabled(m_pageId)) {
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

  const bool autoMarginsEnabled = m_settings->isPageAutoMarginsEnabled(m_pageId);
  for (const PageId& page_id : pages) {
    if (page_id == m_pageId) {
      continue;
    }

    m_settings->setPageAutoMarginsEnabled(page_id, autoMarginsEnabled);
    if (autoMarginsEnabled) {
      m_settings->invalidateContentSize(page_id);
    } else {
      m_settings->setHardMarginsMM(page_id, m_marginsMM);
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

    m_settings->setPageAlignment(page_id, m_alignment);
  }

  emit invalidateAllThumbnails();
}

void OptionsWidget::updateMarginsDisplay() {
  const ScopedIncDec<int> ignoreScope(m_ignoreMarginChanges);

  double topMarginValue = m_marginsMM.top();
  double bottomMarginValue = m_marginsMM.bottom();
  double leftMarginValue = m_marginsMM.left();
  double rightMarginValue = m_marginsMM.right();
  UnitsProvider::getInstance().convertFrom(leftMarginValue, topMarginValue, MILLIMETRES, m_dpi);
  UnitsProvider::getInstance().convertFrom(rightMarginValue, bottomMarginValue, MILLIMETRES, m_dpi);

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
  const bool enabled = !m_settings->isPageAutoMarginsEnabled(m_pageId);

  topMarginSpinBox->setEnabled(enabled);
  bottomMarginSpinBox->setEnabled(enabled);
  leftMarginSpinBox->setEnabled(enabled);
  rightMarginSpinBox->setEnabled(enabled);
  topBottomLink->setEnabled(enabled);
  leftRightLink->setEnabled(enabled);
}

#define CONNECT(...) m_connectionList.push_back(connect(__VA_ARGS__))

void OptionsWidget::setupUiConnections() {
  CONNECT(topMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  CONNECT(bottomMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(vertMarginsChanged(double)));
  CONNECT(leftMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  CONNECT(rightMarginSpinBox, SIGNAL(valueChanged(double)), this, SLOT(horMarginsChanged(double)));
  CONNECT(autoMargins, SIGNAL(toggled(bool)), this, SLOT(autoMarginsToggled(bool)));
  CONNECT(alignmentMode, SIGNAL(currentIndexChanged(int)), this, SLOT(alignmentModeChanged(int)));
  CONNECT(topBottomLink, SIGNAL(clicked()), this, SLOT(topBottomLinkClicked()));
  CONNECT(leftRightLink, SIGNAL(clicked()), this, SLOT(leftRightLinkClicked()));
  CONNECT(applyMarginsBtn, SIGNAL(clicked()), this, SLOT(showApplyMarginsDialog()));
  CONNECT(alignWithOthersCB, SIGNAL(toggled(bool)), this, SLOT(alignWithOthersToggled()));
  CONNECT(applyAlignmentBtn, SIGNAL(clicked()), this, SLOT(showApplyAlignmentDialog()));
  CONNECT(autoHorizontalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoHorizontalAligningToggled(bool)));
  CONNECT(autoVerticalAligningCB, SIGNAL(toggled(bool)), this, SLOT(autoVerticalAligningToggled(bool)));
  for (const auto& kv : m_alignmentByButton) {
    CONNECT(kv.first, SIGNAL(clicked()), this, SLOT(alignmentButtonClicked()));
  }
}

#undef CONNECT

void OptionsWidget::removeUiConnections() {
  for (const auto& connection : m_connectionList) {
    disconnect(connection);
  }
  m_connectionList.clear();
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
  if (m_ignoreAlignmentButtonsChanges) {
    return;
  }

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
  if (m_ignoreAlignmentButtonsChanges) {
    return;
  }

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
  const ScopedIncDec<int> scopeGuard(m_ignoreAlignmentButtonsChanges);

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

void OptionsWidget::updateAlignmentModeEnabled() {
  const bool isAlignmentNull = m_alignment.isNull();

  alignmentMode->setEnabled(!isAlignmentNull);
  autoAlignSettingsGroup->setEnabled(!isAlignmentNull);
}

void OptionsWidget::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  topBottomLink->setIcon(iconProvider.getIcon("ver_chain"));
  leftRightLink->setIcon(iconProvider.getIcon("ver_chain"));
  alignTopLeftBtn->setIcon(iconProvider.getIcon("stock-gravity-north-west"));
  alignTopBtn->setIcon(iconProvider.getIcon("stock-gravity-north"));
  alignTopRightBtn->setIcon(iconProvider.getIcon("stock-gravity-north-east"));
  alignRightBtn->setIcon(iconProvider.getIcon("stock-gravity-east"));
  alignBottomRightBtn->setIcon(iconProvider.getIcon("stock-gravity-south-east"));
  alignBottomBtn->setIcon(iconProvider.getIcon("stock-gravity-south"));
  alignBottomLeftBtn->setIcon(iconProvider.getIcon("stock-gravity-south-west"));
  alignLeftBtn->setIcon(iconProvider.getIcon("stock-gravity-west"));
  alignCenterBtn->setIcon(iconProvider.getIcon("stock-center"));
  m_chainIcon = iconProvider.getIcon("stock-vchain");
  m_brokenChainIcon = iconProvider.getIcon("stock-vchain-broken");
}
}  // namespace page_layout
