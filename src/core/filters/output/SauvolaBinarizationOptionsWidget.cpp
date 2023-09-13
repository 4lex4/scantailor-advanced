// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SauvolaBinarizationOptionsWidget.h"

#include <utility>


namespace output {

SauvolaBinarizationOptionsWidget::SauvolaBinarizationOptionsWidget(std::shared_ptr<Settings> settings)
    : m_settings(std::move(settings)),
      m_connectionManager(std::bind(&SauvolaBinarizationOptionsWidget::setupUiConnections, this)) {
  setupUi(this);

  m_delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void SauvolaBinarizationOptionsWidget::updateUi(const PageId& pageId) {
  auto block = m_connectionManager.getScopedBlock();

  const Params params(m_settings->getParams(pageId));
  m_pageId = pageId;
  m_colorParams = params.colorParams();
  m_outputProcessingParams = m_settings->getOutputProcessingParams(pageId);

  updateView();
}

void SauvolaBinarizationOptionsWidget::sauvolaDeltaChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setThresholdAdjustment(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void SauvolaBinarizationOptionsWidget::windowSizeChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWindowSize(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void SauvolaBinarizationOptionsWidget::sauvolaCoefChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setSauvolaCoef(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void SauvolaBinarizationOptionsWidget::updateView() {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  sauvolaDelta->setValue(blackWhiteOptions.thresholdAdjustment());
  windowSize->setValue(blackWhiteOptions.getWindowSize());
  sauvolaCoef->setValue(blackWhiteOptions.getSauvolaCoef());
}

void SauvolaBinarizationOptionsWidget::sendStateChanged() {
  emit stateChanged();
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void SauvolaBinarizationOptionsWidget::setupUiConnections() {
  CONNECT(sauvolaDelta, SIGNAL(valueChanged(double)), this, SLOT(sauvolaDeltaChanged(double)));
  CONNECT(windowSize, SIGNAL(valueChanged(int)), this, SLOT(windowSizeChanged(int)));
  CONNECT(sauvolaCoef, SIGNAL(valueChanged(double)), this, SLOT(sauvolaCoefChanged(double)));
  CONNECT(&m_delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

#undef CONNECT
}  // namespace output
