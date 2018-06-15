/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OUTPUT_OPTIONSWIDGET_H_
#define OUTPUT_OPTIONSWIDGET_H_

#include <QtCore/QObjectCleanupHandler>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtWidgets/QStackedLayout>
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
#include "ui_OutputOptionsWidget.h"

namespace dewarping {
class DistortionModel;
}

namespace output {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OutputOptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageId& page_id);

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

  intrusive_ptr<Settings> m_ptrSettings;
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
  QTimer delayedReloadRequest;
};
}  // namespace output
#endif  // ifndef OUTPUT_OPTIONSWIDGET_H_
