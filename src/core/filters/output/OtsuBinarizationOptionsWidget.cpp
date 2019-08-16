// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OtsuBinarizationOptionsWidget.h"
#include <foundation/ScopedIncDec.h>
#include <QtWidgets/QToolTip>
#include <utility>
#include "../../Utils.h"

using namespace core;

namespace output {

OtsuBinarizationOptionsWidget::OtsuBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
    : m_settings(std::move(settings)), m_ignoreSliderChanges(0) {
  setupUi(this);

  darkerThresholdLink->setText(Utils::richTextForLink(darkerThresholdLink->text()));
  lighterThresholdLink->setText(Utils::richTextForLink(lighterThresholdLink->text()));
  thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));

  thresholdSlider->setMinimum(-100);
  thresholdSlider->setMaximum(100);
  thresholLabel->setText(QString::number(thresholdSlider->value()));

  m_delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void OtsuBinarizationOptionsWidget::updateUi(const PageId& page_id) {
  removeUiConnections();

  const Params params(m_settings->getParams(page_id));
  m_pageId = page_id;
  m_colorParams = params.colorParams();

  updateView();

  setupUiConnections();
}

void OtsuBinarizationOptionsWidget::thresholdSliderReleased() {
  const int value = thresholdSlider->value();
  setThresholdAdjustment(value);

  emit stateChanged();
}

void OtsuBinarizationOptionsWidget::thresholdSliderValueChanged(int value) {
  if (m_ignoreSliderChanges) {
    return;
  }

  thresholLabel->setText(QString::number(value));

  const QString tooltip_text(QString::number(value));
  thresholdSlider->setToolTip(tooltip_text);

  // Show the tooltip immediately.
  const QPoint center(thresholdSlider->rect().center());
  QPoint tooltip_pos(thresholdSlider->mapFromGlobal(QCursor::pos()));
  tooltip_pos.setY(center.y());
  tooltip_pos.setX(qBound(0, tooltip_pos.x(), thresholdSlider->width()));
  tooltip_pos = thresholdSlider->mapToGlobal(tooltip_pos);
  QToolTip::showText(tooltip_pos, tooltip_text, thresholdSlider);

  if (thresholdSlider->isSliderDown()) {
    return;
  }

  setThresholdAdjustment(value);

  m_delayedStateChanger.start(750);
}

void OtsuBinarizationOptionsWidget::setThresholdAdjustment(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  if (opt.thresholdAdjustment() == value) {
    return;
  }

  thresholLabel->setText(QString::number(value));

  opt.setThresholdAdjustment(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);
}

void OtsuBinarizationOptionsWidget::setLighterThreshold() {
  const ScopedIncDec<int> scopeGuard(m_ignoreSliderChanges);

  thresholdSlider->setValue(thresholdSlider->value() - 1);
  setThresholdAdjustment(thresholdSlider->value());

  m_delayedStateChanger.start(750);
}

void OtsuBinarizationOptionsWidget::setDarkerThreshold() {
  const ScopedIncDec<int> scopeGuard(m_ignoreSliderChanges);

  thresholdSlider->setValue(thresholdSlider->value() + 1);
  setThresholdAdjustment(thresholdSlider->value());

  m_delayedStateChanger.start(750);
}

void OtsuBinarizationOptionsWidget::setNeutralThreshold() {
  const ScopedIncDec<int> scopeGuard(m_ignoreSliderChanges);

  thresholdSlider->setValue(0);
  setThresholdAdjustment(thresholdSlider->value());

  emit stateChanged();
}

void OtsuBinarizationOptionsWidget::updateView() {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  thresholdSlider->setValue(blackWhiteOptions.thresholdAdjustment());
  thresholLabel->setText(QString::number(blackWhiteOptions.thresholdAdjustment()));
}

#define CONNECT(...) m_connectionList.push_back(connect(__VA_ARGS__))

void OtsuBinarizationOptionsWidget::setupUiConnections() {
  CONNECT(lighterThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setLighterThreshold()));
  CONNECT(darkerThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setDarkerThreshold()));
  CONNECT(thresholdSlider, SIGNAL(sliderReleased()), this, SLOT(thresholdSliderReleased()));
  CONNECT(thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(thresholdSliderValueChanged(int)));
  CONNECT(neutralThresholdBtn, SIGNAL(clicked()), this, SLOT(setNeutralThreshold()));
  CONNECT(&m_delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

#undef CONNECT

void OtsuBinarizationOptionsWidget::removeUiConnections() {
  for (const auto& connection : m_connectionList) {
    disconnect(connection);
  }
  m_connectionList.clear();
}

void OtsuBinarizationOptionsWidget::sendStateChanged() {
  emit stateChanged();
}
}  // namespace output