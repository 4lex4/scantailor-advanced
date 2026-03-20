// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidgetBinarizationOtsu.h"

#include <foundation/ScopedIncDec.h>

#include <QtWidgets/QToolTip>
#include <utility>

#include "../../Utils.h"

using namespace core;

namespace output {

OptionsWidgetBinarizationOtsu::OptionsWidgetBinarizationOtsu(std::shared_ptr<Settings> settings)
    : m_settings(std::move(settings)),
      m_connectionManager(std::bind(&OptionsWidgetBinarizationOtsu::setupUiConnections, this)) {
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

void OptionsWidgetBinarizationOtsu::updateUi(const PageId& pageId) {
  auto block = m_connectionManager.getScopedBlock();

  const Params params(m_settings->getParams(pageId));
  m_pageId = pageId;
  m_colorParams = params.colorParams();

  updateView();
}

void OptionsWidgetBinarizationOtsu::thresholdSliderReleased() {
  const int value = thresholdSlider->value();
  setThresholdAdjustment(value);

  emit stateChanged();
}

void OptionsWidgetBinarizationOtsu::thresholdSliderValueChanged(int value) {
  thresholLabel->setText(QString::number(value));

  const QString tooltipText(QString::number(value));
  thresholdSlider->setToolTip(tooltipText);

  // Show the tooltip immediately.
  const QPoint center(thresholdSlider->rect().center());
  QPoint tooltipPos(thresholdSlider->mapFromGlobal(QCursor::pos()));
  tooltipPos.setY(center.y());
  tooltipPos.setX(qBound(0, tooltipPos.x(), thresholdSlider->width()));
  tooltipPos = thresholdSlider->mapToGlobal(tooltipPos);
  QToolTip::showText(tooltipPos, tooltipText, thresholdSlider);

  if (thresholdSlider->isSliderDown()) {
    return;
  }

  setThresholdAdjustment(value);

  m_delayedStateChanger.start(750);
}

void OptionsWidgetBinarizationOtsu::setThresholdAdjustment(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  if (opt.thresholdAdjustment() == value) {
    return;
  }

  thresholLabel->setText(QString::number(value));

  opt.setThresholdAdjustment(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);
}

void OptionsWidgetBinarizationOtsu::setLighterThreshold() {
  auto block = m_connectionManager.getScopedBlock();

  thresholdSlider->setValue(thresholdSlider->value() - 1);
  setThresholdAdjustment(thresholdSlider->value());

  m_delayedStateChanger.start(750);
}

void OptionsWidgetBinarizationOtsu::setDarkerThreshold() {
  auto block = m_connectionManager.getScopedBlock();

  thresholdSlider->setValue(thresholdSlider->value() + 1);
  setThresholdAdjustment(thresholdSlider->value());

  m_delayedStateChanger.start(750);
}

void OptionsWidgetBinarizationOtsu::setNeutralThreshold() {
  auto block = m_connectionManager.getScopedBlock();

  thresholdSlider->setValue(0);
  setThresholdAdjustment(thresholdSlider->value());

  emit stateChanged();
}

void OptionsWidgetBinarizationOtsu::updateView() {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  thresholdSlider->setValue(blackWhiteOptions.thresholdAdjustment());
  thresholLabel->setText(QString::number(blackWhiteOptions.thresholdAdjustment()));
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void OptionsWidgetBinarizationOtsu::setupUiConnections() {
  CONNECT(lighterThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setLighterThreshold()));
  CONNECT(darkerThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setDarkerThreshold()));
  CONNECT(thresholdSlider, SIGNAL(sliderReleased()), this, SLOT(thresholdSliderReleased()));
  CONNECT(thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(thresholdSliderValueChanged(int)));
  CONNECT(neutralThresholdBtn, SIGNAL(clicked()), this, SLOT(setNeutralThreshold()));
  CONNECT(&m_delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

#undef CONNECT

void OptionsWidgetBinarizationOtsu::sendStateChanged() {
  emit stateChanged();
}
}  // namespace output