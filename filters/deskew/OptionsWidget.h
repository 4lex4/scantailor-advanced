/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef DESKEW_OPTIONSWIDGET_H_
#define DESKEW_OPTIONSWIDGET_H_

#include <set>
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "FilterOptionsWidget.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "intrusive_ptr.h"
#include "ui_DeskewOptionsWidget.h"

namespace deskew {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::DeskewOptionsWidget {
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


  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor);

  ~OptionsWidget() override;

 signals:

  void manualDeskewAngleSet(double degrees);

 public slots:

  void manualDeskewAngleSetExternally(double degrees);

 public:
  void preUpdateUI(const PageId& page_id);

  void postUpdateUI(const UiData& ui_data);

 private slots:

  void spinBoxValueChanged(double skew_degrees);

  void modeChanged(bool auto_mode);

  void showDeskewDialog();

  void appliedTo(const std::set<PageId>& pages);

  void appliedToAllPages(const std::set<PageId>& pages);

 private:
  void updateModeIndication(AutoManualMode mode);

  void setSpinBoxUnknownState();

  void setSpinBoxKnownState(double angle);

  void commitCurrentParams();

  void setupUiConnections();

  void removeUiConnections();

  static double spinBoxToDegrees(double sb_value);

  static double degreesToSpinBox(double degrees);

  static const double MAX_ANGLE;

  intrusive_ptr<Settings> m_ptrSettings;
  PageId m_pageId;
  UiData m_uiData;
  int m_ignoreAutoManualToggle;
  int m_ignoreSpinBoxChanges;

  PageSelectionAccessor m_pageSelectionAccessor;
};
}  // namespace deskew
#endif  // ifndef DESKEW_OPTIONSWIDGET_H_
