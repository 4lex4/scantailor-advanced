

#include "WolfBinarizationOptionsWidget.h"

#include <utility>

namespace output {

WolfBinarizationOptionsWidget::WolfBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
    : m_ptrSettings(std::move(settings)) {
  setupUi(this);

  delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void WolfBinarizationOptionsWidget::updateUi(const PageId& page_id) {
  removeUiConnections();

  const Params params(m_ptrSettings->getParams(page_id));
  m_pageId = page_id;
  m_colorParams = params.colorParams();
  m_outputProcessingParams = m_ptrSettings->getOutputProcessingParams(page_id);

  updateView();

  setupUiConnections();
}

void WolfBinarizationOptionsWidget::windowSizeChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWindowSize(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_ptrSettings->setColorParams(m_pageId, m_colorParams);

  delayedStateChanger.start(750);
}

void WolfBinarizationOptionsWidget::lowerBoundChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWolfLowerBound(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_ptrSettings->setColorParams(m_pageId, m_colorParams);

  delayedStateChanger.start(750);
}

void WolfBinarizationOptionsWidget::upperBoundChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWolfUpperBound(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_ptrSettings->setColorParams(m_pageId, m_colorParams);

  delayedStateChanger.start(750);
}

void WolfBinarizationOptionsWidget::wolfCoefChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWolfCoef(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_ptrSettings->setColorParams(m_pageId, m_colorParams);

  delayedStateChanger.start(750);
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

#define CONNECT(...) m_connectionList.push_back(connect(__VA_ARGS__));

void WolfBinarizationOptionsWidget::setupUiConnections() {
  CONNECT(windowSize, SIGNAL(valueChanged(int)), this, SLOT(windowSizeChanged(int)));
  CONNECT(lowerBound, SIGNAL(valueChanged(int)), this, SLOT(lowerBoundChanged(int)));
  CONNECT(upperBound, SIGNAL(valueChanged(int)), this, SLOT(upperBoundChanged(int)));
  CONNECT(wolfCoef, SIGNAL(valueChanged(double)), this, SLOT(wolfCoefChanged(double)));
  CONNECT(&delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

#undef CONNECT

void WolfBinarizationOptionsWidget::removeUiConnections() {
  for (const auto& connection : m_connectionList) {
    disconnect(connection);
  }
  m_connectionList.clear();
}
}  // namespace output