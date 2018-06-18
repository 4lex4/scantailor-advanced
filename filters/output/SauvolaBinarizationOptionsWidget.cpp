
#include "SauvolaBinarizationOptionsWidget.h"

#include <utility>


namespace output {

SauvolaBinarizationOptionsWidget::SauvolaBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
    : m_settings(std::move(settings)) {
  setupUi(this);

  delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void SauvolaBinarizationOptionsWidget::updateUi(const PageId& page_id) {
  removeUiConnections();

  const Params params(m_settings->getParams(page_id));
  m_pageId = page_id;
  m_colorParams = params.colorParams();
  m_outputProcessingParams = m_settings->getOutputProcessingParams(page_id);

  updateView();

  setupUiConnections();
}

void SauvolaBinarizationOptionsWidget::windowSizeChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWindowSize(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  delayedStateChanger.start(750);
}

void SauvolaBinarizationOptionsWidget::sauvolaCoefChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setSauvolaCoef(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  delayedStateChanger.start(750);
}

void SauvolaBinarizationOptionsWidget::updateView() {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  windowSize->setValue(blackWhiteOptions.getWindowSize());
  sauvolaCoef->setValue(blackWhiteOptions.getSauvolaCoef());
}

void SauvolaBinarizationOptionsWidget::sendStateChanged() {
  emit stateChanged();
}

#define CONNECT(...) m_connectionList.push_back(connect(__VA_ARGS__));

void SauvolaBinarizationOptionsWidget::setupUiConnections() {
  CONNECT(windowSize, SIGNAL(valueChanged(int)), this, SLOT(windowSizeChanged(int)));
  CONNECT(sauvolaCoef, SIGNAL(valueChanged(double)), this, SLOT(sauvolaCoefChanged(double)));
  CONNECT(&delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

#undef CONNECT

void SauvolaBinarizationOptionsWidget::removeUiConnections() {
  for (const auto& connection : m_connectionList) {
    disconnect(connection);
  }
  m_connectionList.clear();
}
}  // namespace output