// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "WolfBinarizationOptionsWidget.h"

#include <utility>

namespace output {

WolfBinarizationOptionsWidget::WolfBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
    : m_settings(std::move(settings)) {
  setupUi(this);

  m_delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void WolfBinarizationOptionsWidget::updateUi(const PageId& pageId) {
  removeUiConnections();

  const Params params(m_settings->getParams(pageId));
  m_pageId = pageId;
  m_colorParams = params.colorParams();
  m_outputProcessingParams = m_settings->getOutputProcessingParams(pageId);

  updateView();

  setupUiConnections();
}

void WolfBinarizationOptionsWidget::windowSizeChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWindowSize(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void WolfBinarizationOptionsWidget::lowerBoundChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWolfLowerBound(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void WolfBinarizationOptionsWidget::upperBoundChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWolfUpperBound(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void WolfBinarizationOptionsWidget::wolfCoefChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWolfCoef(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void WolfBinarizationOptionsWidget::updateView() {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  windowSize->setValue(blackWhiteOptions.getWindowSize());
  lowerBound->setValue(blackWhiteOptions.getWolfLowerBound());
  upperBound->setValue(blackWhiteOptions.getWolfUpperBound());
  wolfCoef->setValue(blackWhiteOptions.getWolfCoef());
}

void WolfBinarizationOptionsWidget::sendStateChanged() {
  emit stateChanged();
}

#define CONNECT(...) m_connectionList.push_back(connect(__VA_ARGS__))

void WolfBinarizationOptionsWidget::setupUiConnections() {
  CONNECT(windowSize, SIGNAL(valueChanged(int)), this, SLOT(windowSizeChanged(int)));
  CONNECT(lowerBound, SIGNAL(valueChanged(int)), this, SLOT(lowerBoundChanged(int)));
  CONNECT(upperBound, SIGNAL(valueChanged(int)), this, SLOT(upperBoundChanged(int)));
  CONNECT(wolfCoef, SIGNAL(valueChanged(double)), this, SLOT(wolfCoefChanged(double)));
  CONNECT(&m_delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

#undef CONNECT

void WolfBinarizationOptionsWidget::removeUiConnections() {
  for (const auto& connection : m_connectionList) {
    disconnect(connection);
  }
  m_connectionList.clear();
}
}  // namespace output