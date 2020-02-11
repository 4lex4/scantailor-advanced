// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_OPTIONSWIDGET_H_
#define SCANTAILOR_DESKEW_OPTIONSWIDGET_H_

#include <core/ConnectionManager.h>

#include <list>
#include <memory>
#include <set>

#include "AutoManualMode.h"
#include "Dependencies.h"
#include "FilterOptionsWidget.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "ui_OptionsWidget.h"

namespace deskew {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  class UiData {
    // Member-wise copying is OK.
   public:
    UiData();

    ~UiData();

    void setEffectiveDeskewAngle(double degrees);

    double effectiveDeskewAngle() const;

    void setDependencies(const Dependencies& deps);

    const Dependencies& dependencies() const;

    void setMode(AutoManualMode mode);

    AutoManualMode mode() const;

   private:
    double m_effDeskewAngle;
    Dependencies m_deps;
    AutoManualMode m_mode;
  };


  OptionsWidget(std::shared_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

 signals:

  void manualDeskewAngleSet(double degrees);

 public slots:

  void manualDeskewAngleSetExternally(double degrees);

 public:
  void preUpdateUI(const PageId& pageId);

  void postUpdateUI(const UiData& uiData);

 private slots:

  void spinBoxValueChanged(double skewDegrees);

  void modeChanged(bool autoMode);

  void showDeskewDialog();

  void appliedTo(const std::set<PageId>& pages);

  void appliedToAllPages(const std::set<PageId>& pages);

 private:
  void updateModeIndication(AutoManualMode mode);

  void setSpinBoxUnknownState();

  void setSpinBoxKnownState(double angle);

  void commitCurrentParams();

  void setupUiConnections();

  static double spinBoxToDegrees(double sbValue);

  static double degreesToSpinBox(double degrees);

  static const double MAX_ANGLE;

  std::shared_ptr<Settings> m_settings;
  PageId m_pageId;
  UiData m_uiData;
  int m_ignoreAutoManualToggle;
  int m_ignoreSpinBoxChanges;

  PageSelectionAccessor m_pageSelectionAccessor;

  ConnectionManager m_connectionManager;
};
}  // namespace deskew
#endif  // ifndef SCANTAILOR_DESKEW_OPTIONSWIDGET_H_
