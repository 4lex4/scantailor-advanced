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

#ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
#define SELECT_CONTENT_OPTIONSWIDGET_H_

#include <UnitsObserver.h>
#include <QRectF>
#include <QSizeF>
#include <memory>
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "FilterOptionsWidget.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "Params.h"
#include "PhysSizeCalc.h"
#include "intrusive_ptr.h"
#include "ui_SelectContentOptionsWidget.h"

namespace select_content {
class Settings;

class OptionsWidget : public FilterOptionsWidget, public UnitsObserver, private Ui::SelectContentOptionsWidget {
  Q_OBJECT
 public:
  class UiData {
    // Member-wise copying is OK.
   public:
    UiData();

    ~UiData();

    void setSizeCalc(const PhysSizeCalc& calc);

    void setContentRect(const QRectF& content_rect);

    void setPageRect(const QRectF& content_rect);

    const QRectF& contentRect() const;

    const QRectF& pageRect() const;

    QSizeF contentSizeMM() const;

    void setDependencies(const Dependencies& deps);

    const Dependencies& dependencies() const;

    void setContentDetectionMode(AutoManualMode mode);

    void setPageDetectionMode(AutoManualMode mode);

    bool isContentDetectionEnabled() const;

    bool isPageDetectionEnabled() const;

    bool isFineTuningCornersEnabled() const;

    void setContentDetectionEnabled(bool detect);

    void setPageDetectionEnabled(bool detect);

    void setFineTuneCornersEnabled(bool fine_tune);

    AutoManualMode contentDetectionMode() const;

    AutoManualMode pageDetectionMode() const;

   private:
    QRectF m_contentRect;  // In virtual image coordinates.
    QRectF m_pageRect;
    PhysSizeCalc m_sizeCalc;
    Dependencies m_deps;
    AutoManualMode m_contentDetectionMode;
    AutoManualMode m_pageDetectionMode;
    bool m_contentDetectionEnabled;
    bool m_pageDetectionEnabled;
    bool m_fineTuneCornersEnabled;
  };


  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageInfo& page_info);

  void postUpdateUI(const UiData& ui_data);

  void updateUnits(Units units) override;

 public slots:

  void manualContentRectSet(const QRectF& content_rect);

  void manualPageRectSet(const QRectF& page_rect);

  void updatePageRectSize(const QSizeF& size);

 signals:

  void pageRectChangedLocally(const QRectF& pageRect);

  void pageRectStateChanged(bool state);

 private slots:

  void showApplyToDialog();

  void applySelection(const std::set<PageId>& pages, bool apply_content_box, bool apply_page_box);

  void contentDetectAutoToggled();

  void contentDetectManualToggled();

  void contentDetectDisableToggled();

  void pageDetectAutoToggled();

  void pageDetectManualToggled();

  void pageDetectDisableToggled();

  void fineTuningChanged(bool checked);

  void dimensionsChangedLocally(double);

 private:
  void updateContentModeIndication(AutoManualMode mode);

  void updatePageModeIndication(AutoManualMode mode);

  void updatePageDetectOptionsDisplay();

  void commitCurrentParams();

  void setupUiConnections();

  void removeUiConnections();

  intrusive_ptr<Settings> m_ptrSettings;
  UiData m_uiData;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  Dpi m_dpi;
  int m_ignorePageSizeChanges;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
