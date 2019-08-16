// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
#define SELECT_CONTENT_OPTIONSWIDGET_H_

#include <AutoManualMode.h>
#include <UnitsListener.h>
#include <QRectF>
#include <QSizeF>
#include <list>
#include <memory>
#include "Dependencies.h"
#include "FilterOptionsWidget.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "Params.h"
#include "PhysSizeCalc.h"
#include "intrusive_ptr.h"
#include "ui_OptionsWidget.h"

namespace select_content {
class Settings;

class OptionsWidget : public FilterOptionsWidget, public UnitsListener, private Ui::OptionsWidget {
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

    bool isFineTuningCornersEnabled() const;

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
    bool m_fineTuneCornersEnabled;
  };


  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageInfo& page_info);

  void postUpdateUI(const UiData& ui_data);

  void onUnitsChanged(Units units) override;

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

  void contentDetectToggled(AutoManualMode mode);

  void pageDetectToggled(AutoManualMode mode);

  void fineTuningChanged(bool checked);

  void dimensionsChangedLocally(double);

 private:
  void updateContentModeIndication(AutoManualMode mode);

  void updatePageModeIndication(AutoManualMode mode);

  void updatePageDetectOptionsDisplay();

  void commitCurrentParams();

  void updateDependenciesIfNecessary();

  void setupUiConnections();

  void removeUiConnections();

  intrusive_ptr<Settings> m_settings;
  UiData m_uiData;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  Dpi m_dpi;
  int m_ignorePageSizeChanges;

  std::list<QMetaObject::Connection> m_connectionList;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
