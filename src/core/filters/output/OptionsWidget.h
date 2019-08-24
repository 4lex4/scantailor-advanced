// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OPTIONSWIDGET_H_
#define SCANTAILOR_OUTPUT_OPTIONSWIDGET_H_

#include <QtCore/QObjectCleanupHandler>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtWidgets/QStackedLayout>
#include <list>
#include <memory>
#include <set>
#include "BinarizationOptionsWidget.h"
#include "ColorParams.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "DewarpingOptions.h"
#include "Dpi.h"
#include "FilterOptionsWidget.h"
#include "ImageViewTab.h"
#include "OutputProcessingParams.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "Params.h"
#include "intrusive_ptr.h"
#include "ui_OptionsWidget.h"

namespace dewarping {
class DistortionModel;
}

namespace output {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageId& pageId);

  void postUpdateUI();

  ImageViewTab lastTab() const;

  const DepthPerception& depthPerception() const;

 signals:

  void despeckleLevelChanged(double level, bool* handled);

  void depthPerceptionChanged(double val);

 public slots:

  void tabChanged(ImageViewTab tab);

  void distortionModelChanged(const dewarping::DistortionModel& model);

 private slots:

  void changeDpiButtonClicked();

  void applyColorsButtonClicked();

  void applySplittingButtonClicked();

  void dpiChanged(const std::set<PageId>& pages, const Dpi& dpi);

  void applyColorsConfirmed(const std::set<PageId>& pages);

  void applySplittingOptionsConfirmed(const std::set<PageId>& pages);

  void colorModeChanged(int idx);

  void blackOnWhiteToggled(bool value);

  void applyProcessingParamsClicked();

  void applyProcessingParamsConfirmed(const std::set<PageId>& pages);

  void thresholdMethodChanged(int idx);

  void fillingColorChanged(int idx);

  void pictureShapeChanged(int idx);

  void pictureShapeSensitivityChanged(int value);

  void higherSearchSensivityToggled(bool checked);

  void colorSegmentationToggled(bool checked);

  void reduceNoiseChanged(int value);

  void redAdjustmentChanged(int value);

  void greenAdjustmentChanged(int value);

  void blueAdjustmentChanged(int value);

  void posterizeToggled(bool checked);

  void posterizeLevelChanged(int value);

  void posterizeNormalizationToggled(bool checked);

  void posterizeForceBwToggled(bool checked);

  void fillMarginsToggled(bool checked);

  void fillOffcutToggled(bool checked);

  void equalizeIlluminationToggled(bool checked);

  void equalizeIlluminationColorToggled(bool checked);

  void savitzkyGolaySmoothingToggled(bool checked);

  void morphologicalSmoothingToggled(bool checked);

  void splittingToggled(bool checked);

  void bwForegroundToggled(bool checked);

  void colorForegroundToggled(bool checked);

  void originalBackgroundToggled(bool checked);

  void binarizationSettingsChanged();

  void despeckleToggled(bool checked);

  void despeckleSliderReleased();

  void despeckleSliderValueChanged(int value);

  void applyDespeckleButtonClicked();

  void applyDespeckleConfirmed(const std::set<PageId>& pages);

  void changeDewarpingButtonClicked();

  void dewarpingChanged(const std::set<PageId>& pages, const DewarpingOptions& opt);

  void applyDepthPerceptionButtonClicked();

  void applyDepthPerceptionConfirmed(const std::set<PageId>& pages);

  void depthPerceptionChangedSlot(int val);

  void updateBinarizationOptionsDisplay(int idx);

  void sendReloadRequested();

 private:
  void handleDespeckleLevelChange(double level, bool delay = false);

  void reloadIfNecessary();

  void updateDpiDisplay();

  void updateColorsDisplay();

  void updateDewarpingDisplay();

  void updateProcessingDisplay();

  void addBinarizationOptionsWidget(BinarizationOptionsWidget* widget);

  void setupUiConnections();

  void removeUiConnections();

  intrusive_ptr<Settings> m_settings;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  Dpi m_outputDpi;
  ColorParams m_colorParams;
  SplittingOptions m_splittingOptions;
  PictureShapeOptions m_pictureShapeOptions;
  DepthPerception m_depthPerception;
  DewarpingOptions m_dewarpingOptions;
  double m_despeckleLevel;
  ImageViewTab m_lastTab;
  QTimer m_delayedReloadRequest;

  std::list<QMetaObject::Connection> m_connectionList;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_OPTIONSWIDGET_H_
