// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OTSUBINARIZATIONOPTIONSWIDGET_H_
#define SCANTAILOR_OUTPUT_OTSUBINARIZATIONOPTIONSWIDGET_H_

#include <core/ConnectionManager.h>

#include <QtCore>
#include <list>
#include <memory>

#include "BinarizationOptionsWidget.h"
#include "ColorParams.h"
#include "Settings.h"
#include "ui_OtsuBinarizationOptionsWidget.h"

namespace output {
class OtsuBinarizationOptionsWidget : public BinarizationOptionsWidget, private Ui::OtsuBinarizationOptionsWidget {
  Q_OBJECT
 public:
  explicit OtsuBinarizationOptionsWidget(std::shared_ptr<Settings> settings);

  ~OtsuBinarizationOptionsWidget() override = default;

  void updateUi(const PageId& m_pageId) override;

 private slots:

  void thresholdSliderReleased();

  void thresholdSliderValueChanged(int value);

  void setLighterThreshold();

  void setDarkerThreshold();

  void setNeutralThreshold();

  void sendStateChanged();

 private:
  void updateView();

  void setThresholdAdjustment(int value);

  void setupUiConnections();

  std::shared_ptr<Settings> m_settings;
  PageId m_pageId;
  ColorParams m_colorParams;
  QTimer m_delayedStateChanger;

  ConnectionManager m_connectionManager;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUT_OTSUBINARIZATIONOPTIONSWIDGET_H_
