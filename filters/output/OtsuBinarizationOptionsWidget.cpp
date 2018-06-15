
#include "OtsuBinarizationOptionsWidget.h"
#include <foundation/ScopedIncDec.h>
#include <QtWidgets/QToolTip>
#include <utility>
#include "../../Utils.h"

namespace output {

OtsuBinarizationOptionsWidget::OtsuBinarizationOptionsWidget(intrusive_ptr<Settings> settings)
    : m_ptrSettings(std::move(settings)), ignoreSliderChanges(0) {
  setupUi(this);

  darkerThresholdLink->setText(Utils::richTextForLink(darkerThresholdLink->text()));
  lighterThresholdLink->setText(Utils::richTextForLink(lighterThresholdLink->text()));
  thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));

  thresholdSlider->setMinimum(-100);
  thresholdSlider->setMaximum(100);
  thresholLabel->setText(QString::number(thresholdSlider->value()));

  delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void OtsuBinarizationOptionsWidget::updateUi(const PageId& page_id) {
  removeUiConnections();

  const Params params(m_ptrSettings->getParams(page_id));
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
  if (ignoreSliderChanges) {
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

  delayedStateChanger.start(750);
}

void OtsuBinarizationOptionsWidget::setThresholdAdjustment(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  if (opt.thresholdAdjustment() == value) {
    return;
  }

  thresholLabel->setText(QString::number(value));

  opt.setThresholdAdjustment(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_ptrSettings->setColorParams(m_pageId, m_colorParams);
}

void OtsuBinarizationOptionsWidget::setLighterThreshold() {
  const ScopedIncDec<int> scopeGuard(ignoreSliderChanges);

  thresholdSlider->setValue(thresholdSlider->value() - 1);
  setThresholdAdjustment(thresholdSlider->value());

  delayedStateChanger.start(750);
}

void OtsuBinarizationOptionsWidget::setDarkerThreshold() {
  const ScopedIncDec<int> scopeGuard(ignoreSliderChanges);

  thresholdSlider->setValue(thresholdSlider->value() + 1);
  setThresholdAdjustment(thresholdSlider->value());

  delayedStateChanger.start(750);
}

void OtsuBinarizationOptionsWidget::setNeutralThreshold() {
  const ScopedIncDec<int> scopeGuard(ignoreSliderChanges);

  thresholdSlider->setValue(0);
  setThresholdAdjustment(thresholdSlider->value());

  emit stateChanged();
}

void OtsuBinarizationOptionsWidget::updateView() {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  thresholdSlider->setValue(blackWhiteOptions.thresholdAdjustment());
  thresholLabel->setText(QString::number(blackWhiteOptions.thresholdAdjustment()));
}

void OtsuBinarizationOptionsWidget::setupUiConnections() {
  connect(lighterThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setLighterThreshold()));
  connect(darkerThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setDarkerThreshold()));
  connect(thresholdSlider, SIGNAL(sliderReleased()), this, SLOT(thresholdSliderReleased()));
  connect(thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(thresholdSliderValueChanged(int)));
  connect(neutralThresholdBtn, SIGNAL(clicked()), this, SLOT(setNeutralThreshold()));
  connect(&delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

void OtsuBinarizationOptionsWidget::removeUiConnections() {
  disconnect(lighterThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setLighterThreshold()));
  disconnect(darkerThresholdLink, SIGNAL(linkActivated(const QString&)), this, SLOT(setDarkerThreshold()));
  disconnect(thresholdSlider, SIGNAL(sliderReleased()), this, SLOT(thresholdSliderReleased()));
  disconnect(thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(thresholdSliderValueChanged(int)));
  disconnect(neutralThresholdBtn, SIGNAL(clicked()), this, SLOT(setNeutralThreshold()));
  disconnect(&delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

void OtsuBinarizationOptionsWidget::sendStateChanged() {
  emit stateChanged();
}
}  // namespace output