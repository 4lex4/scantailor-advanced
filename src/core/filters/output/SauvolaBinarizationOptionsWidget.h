// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_SAUVOLABINARIZATIONOPTIONSWIDGET_H_
#define SCANTAILOR_OUTPUT_SAUVOLABINARIZATIONOPTIONSWIDGET_H_

#include <QtCore>
#include <list>

#include "BinarizationOptionsWidget.h"
#include "ColorParams.h"
#include "Settings.h"
#include "intrusive_ptr.h"
#include "ui_SauvolaBinarizationOptionsWidget.h"

namespace output {
class SauvolaBinarizationOptionsWidget : public BinarizationOptionsWidget,
                                         private Ui::SauvolaBinarizationOptionsWidget {
  Q_OBJECT
 public:
  explicit SauvolaBinarizationOptionsWidget(intrusive_ptr<Settings> settings);

  ~SauvolaBinarizationOptionsWidget() override = default;

  void updateUi(const PageId& m_pageId) override;

 private slots:

  void windowSizeChanged(int value);

  void sauvolaCoefChanged(double value);

  void sendStateChanged();

 private:
  void updateView();

  void setupUiConnections();

  void removeUiConnections();

  intrusive_ptr<Settings> m_settings;
  PageId m_pageId;
  ColorParams m_colorParams;
  QTimer m_delayedStateChanger;
  OutputProcessingParams m_outputProcessingParams;

  std::list<QMetaObject::Connection> m_connectionList;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUT_SAUVOLABINARIZATIONOPTIONSWIDGET_H_
