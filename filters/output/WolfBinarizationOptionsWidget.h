
#ifndef SCANTAILOR_WOLFBINARIZATIONOPTIONSWIDGET_H
#define SCANTAILOR_WOLFBINARIZATIONOPTIONSWIDGET_H

#include <QtCore>
#include "BinarizationOptionsWidget.h"
#include "ColorParams.h"
#include "Settings.h"
#include "intrusive_ptr.h"
#include "ui_WolfBinarizationOptionsWidget.h"

namespace output {
class WolfBinarizationOptionsWidget : public BinarizationOptionsWidget, private Ui::WolfBinarizationOptionsWidget {
  Q_OBJECT

 private:
  intrusive_ptr<Settings> m_ptrSettings;
  PageId m_pageId;
  ColorParams m_colorParams;
  QTimer delayedStateChanger;
  OutputProcessingParams m_outputProcessingParams;

 public:
  explicit WolfBinarizationOptionsWidget(intrusive_ptr<Settings> settings);

  ~WolfBinarizationOptionsWidget() override = default;

  void updateUi(const PageId& m_pageId) override;

 private slots:

  void windowSizeChanged(int value);

  void wolfCoefChanged(double value);

  void lowerBoundChanged(int value);

  void upperBoundChanged(int value);

  void sendStateChanged();

 private:
  void updateView();

  void setupUiConnections();

  void removeUiConnections();
};
}  // namespace output


#endif  // SCANTAILOR_WOLFBINARIZATIONOPTIONSWIDGET_H
