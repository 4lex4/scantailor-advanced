
#include "SauvolaBinarizationOptionsWidget.h"

#include <utility>


namespace output {

SauvolaBinarizationOptionsWidget::SauvolaBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
    : m_ptrSettings(std::move(settings)) {
  setupUi(this);

  delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void SauvolaBinarizationOptionsWidget::updateUi(const PageId& page_id) {
  removeUiConnections();

  const Params params(m_ptrSettings->getParams(page_id));
  m_pageId = page_id;
  m_colorParams = params.colorParams();
  m_outputProcessingParams = m_ptrSettings->getOutputProcessingParams(page_id);

  updateView();

  setupUiConnections();
}

void SauvolaBinarizationOptionsWidget::windowSizeChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWindowSize(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_ptrSettings->setColorParams(m_pageId, m_colorParams);

  delayedStateChanger.start(750);
}

void SauvolaBinarizationOptionsWidget::sauvolaCoefChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setSauvolaCoef(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_ptrSettings->setColorParams(m_pageId, m_colorParams);

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

void SauvolaBinarizationOptionsWidget::setupUiConnections() {
  connect(windowSize, SIGNAL(valueChanged(int)), this, SLOT(windowSizeChanged(int)));
  connect(sauvolaCoef, SIGNAL(valueChanged(double)), this, SLOT(sauvolaCoefChanged(double)));
  connect(&delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

void SauvolaBinarizationOptionsWidget::removeUiConnections() {
  disconnect(windowSize, SIGNAL(valueChanged(int)), this, SLOT(windowSizeChanged(int)));
  disconnect(sauvolaCoef, SIGNAL(valueChanged(double)), this, SLOT(sauvolaCoefChanged(double)));
  disconnect(&delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}
}  // namespace output