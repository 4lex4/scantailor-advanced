// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_DEFAULTPARAMSDIALOG_H_
#define SCANTAILOR_APP_DEFAULTPARAMSDIALOG_H_

#include <QButtonGroup>
#include <QWidget>
#include <list>
#include <set>
#include <unordered_map>
#include "DefaultParams.h"
#include "DefaultParamsProfileManager.h"
#include "OrthogonalRotation.h"
#include "ui_DefaultParamsDialog.h"

class DefaultParamsDialog : public QDialog, private Ui::DefaultParamsDialog {
  Q_OBJECT
 public:
  explicit DefaultParamsDialog(QWidget* parent = nullptr);

  ~DefaultParamsDialog() override = default;

 private slots:

  void rotateLeft();

  void rotateRight();

  void resetRotation();

  void layoutModeChanged(int idx);

  void deskewModeChanged(bool autoMode);

  void pageDetectAutoToggled();

  void pageDetectManualToggled();

  void pageDetectDisableToggled();

  void autoMarginsToggled(bool checked);

  void alignmentModeChanged(int idx);

  void alignWithOthersToggled(bool state);

  void autoHorizontalAligningToggled(bool);

  void autoVerticalAligningToggled(bool);

  void topBottomLinkClicked();

  void leftRightLinkClicked();

  void horMarginsChanged(double val);

  void vertMarginsChanged(double val);

  void colorModeChanged(int idx);

  void thresholdMethodChanged(int idx);

  void pictureShapeChanged(int idx);

  void equalizeIlluminationToggled(bool checked);

  void splittingToggled(bool checked);

  void bwForegroundToggled(bool checked);

  void colorForegroundToggled(bool checked);

  void thresholdSliderValueChanged(int value);

  void colorSegmentationToggled(bool checked);

  void posterizeToggled(bool checked);

  void setLighterThreshold();

  void setDarkerThreshold();

  void setNeutralThreshold();

  void dpiSelectionChanged(int index);

  void dpiEditTextChanged(const QString& text);

  void depthPerceptionChangedSlot(int val);

  void despeckleToggled(bool checked);

  void despeckleSliderValueChanged(int value);

  void profileChanged(int index);

  void profileSavePressed();

  void profileDeletePressed();

  void commitChanges();

 private:
  void updateFixOrientationDisplay(const DefaultParams::FixOrientationParams& params);

  void updatePageSplitDisplay(const DefaultParams::PageSplitParams& params);

  void updateDeskewDisplay(const DefaultParams::DeskewParams& params);

  void updateSelectContentDisplay(const DefaultParams::SelectContentParams& params);

  void updatePageLayoutDisplay(const DefaultParams::PageLayoutParams& params);

  void updateOutputDisplay(const DefaultParams::OutputParams& params);

  void updateUnits(Units units);

  void setupUiConnections();

  void removeUiConnections();

  void setRotation(const OrthogonalRotation& rotation);

  void setRotationPixmap();

  void updateAlignmentButtonsEnabled();

  void updateAutoModeButtons();

  void updateAlignmentModeEnabled();

  QToolButton* getCheckedAlignmentButton() const;

  void setLinkButtonLinked(QToolButton* button, bool linked);

  void loadParams(const DefaultParams& params);

  std::unique_ptr<DefaultParams> buildParams() const;

  bool isProfileNameReserved(const QString& name);

  void setTabWidgetsEnabled(bool enabled);

  void setupIcons();

  QIcon m_chainIcon;
  QIcon m_brokenChainIcon;
  bool m_leftRightLinkEnabled;
  bool m_topBottomLinkEnabled;
  int m_ignoreMarginChanges;
  OrthogonalRotation m_orthogonalRotation;
  std::unordered_map<QToolButton*, page_layout::Alignment> m_alignmentByButton;
  std::unique_ptr<QButtonGroup> m_alignmentButtonGroup;
  DefaultParamsProfileManager m_profileManager;
  int m_customDpiItemIdx;
  QString m_customDpiValue;
  int m_customProfileItemIdx;
  Units m_currentUnits;
  std::set<QString> m_reservedProfileNames;
  int m_ignoreProfileChanges;

  std::list<QMetaObject::Connection> m_connectionList;
};


#endif  // SCANTAILOR_APP_DEFAULTPARAMSDIALOG_H_
