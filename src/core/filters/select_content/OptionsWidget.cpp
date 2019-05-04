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

#include "OptionsWidget.h"
#include "ApplyDialog.h"
#include "ScopedIncDec.h"
#include "Settings.h"

#include <UnitsProvider.h>
#include <boost/bind.hpp>
#include <iostream>
#include <utility>

namespace select_content {
OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor)
    : m_settings(std::move(settings)), m_pageSelectionAccessor(page_selection_accessor), m_ignorePageSizeChanges(0) {
  setupUi(this);

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageInfo& page_info) {
  removeUiConnections();

  m_pageId = page_info.id();
  m_dpi = page_info.metadata().dpi();

  contentBoxGroup->setEnabled(false);
  pageBoxGroup->setEnabled(false);

  pageDetectOptions->setVisible(false);
  fineTuneBtn->setVisible(false);
  dimensionsWidget->setVisible(false);

  updateUnits(UnitsProvider::getInstance().getUnits());

  setupUiConnections();
}

void OptionsWidget::postUpdateUI(const UiData& ui_data) {
  removeUiConnections();

  m_uiData = ui_data;

  updateContentModeIndication(ui_data.contentDetectionMode());
  updatePageModeIndication(ui_data.pageDetectionMode());

  contentBoxGroup->setEnabled(true);
  pageBoxGroup->setEnabled(true);

  updatePageDetectOptionsDisplay();
  updatePageRectSize(m_uiData.pageRect().size());

  setupUiConnections();
}

void OptionsWidget::manualContentRectSet(const QRectF& content_rect) {
  m_uiData.setContentRect(content_rect);
  m_uiData.setContentDetectionMode(MODE_MANUAL);
  updateContentModeIndication(MODE_MANUAL);

  commitCurrentParams();

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::manualPageRectSet(const QRectF& page_rect) {
  m_uiData.setPageRect(page_rect);
  m_uiData.setPageDetectionMode(MODE_MANUAL);
  updatePageModeIndication(MODE_MANUAL);
  updatePageDetectOptionsDisplay();
  updatePageRectSize(page_rect.size());

  commitCurrentParams();

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::updatePageRectSize(const QSizeF& size) {
  const ScopedIncDec<int> ignore_scope(m_ignorePageSizeChanges);

  double width = size.width();
  double height = size.height();
  UnitsProvider::getInstance().convertFrom(width, height, PIXELS, m_dpi);

  widthSpinBox->setValue(width);
  heightSpinBox->setValue(height);
}

void OptionsWidget::contentDetectToggled(const AutoManualMode mode) {
  m_uiData.setContentDetectionMode(mode);
  commitCurrentParams();

  if (mode != MODE_MANUAL) {
    emit reloadRequested();
  }
}

void OptionsWidget::pageDetectToggled(const AutoManualMode mode) {
  const bool need_update_state = ((mode == MODE_MANUAL) && (m_uiData.pageDetectionMode() == MODE_DISABLED));

  m_uiData.setPageDetectionMode(mode);
  updatePageDetectOptionsDisplay();
  commitCurrentParams();

  if (mode != MODE_MANUAL) {
    emit reloadRequested();
  } else if (need_update_state) {
    emit pageRectStateChanged(true);
    emit invalidateThumbnail(m_pageId);
  }
}

void OptionsWidget::fineTuningChanged(bool checked) {
  m_uiData.setFineTuneCornersEnabled(checked);
  commitCurrentParams();
  if (m_uiData.pageDetectionMode() == MODE_AUTO) {
    emit reloadRequested();
  }
}

void OptionsWidget::updateContentModeIndication(const AutoManualMode mode) {
  switch (mode) {
    case MODE_AUTO:
      contentDetectAutoBtn->setChecked(true);
      break;
    case MODE_MANUAL:
      contentDetectManualBtn->setChecked(true);
      break;
    case MODE_DISABLED:
      contentDetectDisableBtn->setChecked(true);
      break;
  }
}

void OptionsWidget::updatePageModeIndication(const AutoManualMode mode) {
  switch (mode) {
    case MODE_AUTO:
      pageDetectAutoBtn->setChecked(true);
      break;
    case MODE_MANUAL:
      pageDetectManualBtn->setChecked(true);
      break;
    case MODE_DISABLED:
      pageDetectDisableBtn->setChecked(true);
      break;
  }
}

void OptionsWidget::updatePageDetectOptionsDisplay() {
  fineTuneBtn->setChecked(m_uiData.isFineTuningCornersEnabled());
  pageDetectOptions->setVisible(m_uiData.pageDetectionMode() != MODE_DISABLED);
  fineTuneBtn->setVisible(m_uiData.pageDetectionMode() == MODE_AUTO);
  dimensionsWidget->setVisible(m_uiData.pageDetectionMode() == MODE_MANUAL);
}

void OptionsWidget::dimensionsChangedLocally(double) {
  if (m_ignorePageSizeChanges) {
    return;
  }

  double widthSpinBoxValue = widthSpinBox->value();
  double heightSpinBoxValue = heightSpinBox->value();
  UnitsProvider::getInstance().convertTo(widthSpinBoxValue, heightSpinBoxValue, PIXELS, m_dpi);

  QRectF newPageRect = m_uiData.pageRect();
  newPageRect.setSize(QSizeF(widthSpinBoxValue, heightSpinBoxValue));

  emit pageRectChangedLocally(newPageRect);
}

void OptionsWidget::commitCurrentParams() {
  updateDependenciesIfNecessary();

  Params params(m_uiData.contentRect(), m_uiData.contentSizeMM(), m_uiData.pageRect(), m_uiData.dependencies(),
                m_uiData.contentDetectionMode(), m_uiData.pageDetectionMode(), m_uiData.isFineTuningCornersEnabled());
  m_settings->setPageParams(m_pageId, params);
}

void OptionsWidget::updateDependenciesIfNecessary() {
  // On switching to manual mode the page dependencies isn't updated
  // as Task::process isn't called, so we need to update it manually.
  if (!(m_uiData.contentDetectionMode() == MODE_MANUAL || m_uiData.pageDetectionMode() == MODE_MANUAL)) {
    return;
  }

  Dependencies deps = m_uiData.dependencies();
  if (m_uiData.contentDetectionMode() == MODE_MANUAL) {
    deps.setContentDetectionMode(MODE_MANUAL);
  }
  if (m_uiData.pageDetectionMode() == MODE_MANUAL) {
    deps.setPageDetectionMode(MODE_MANUAL);
  }
  m_uiData.setDependencies(deps);
}

void OptionsWidget::showApplyToDialog() {
  auto* dialog = new ApplyDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, SIGNAL(applySelection(const std::set<PageId>&, bool, bool)), this,
          SLOT(applySelection(const std::set<PageId>&, bool, bool)));
  dialog->show();
}

void OptionsWidget::applySelection(const std::set<PageId>& pages,
                                   const bool apply_content_box,
                                   const bool apply_page_box) {
  if (pages.empty()) {
    return;
  }

  const Params params(m_uiData.contentRect(), m_uiData.contentSizeMM(), m_uiData.pageRect(), Dependencies(),
                      m_uiData.contentDetectionMode(), m_uiData.pageDetectionMode(),
                      m_uiData.isFineTuningCornersEnabled());

  for (const PageId& page_id : pages) {
    if (m_pageId == page_id) {
      continue;
    }

    Params new_params(params);
    std::unique_ptr<Params> old_params = m_settings->getPageParams(page_id);
    if (old_params) {
      if (new_params.pageDetectionMode() == MODE_MANUAL) {
        if (!apply_page_box) {
          new_params.setPageRect(old_params->pageRect());
        } else {
          QRectF corrected_page_rect = new_params.pageRect();
          const QRectF source_image_rect = new_params.dependencies().rotatedPageOutline().boundingRect();
          const QRectF current_image_rect = old_params->dependencies().rotatedPageOutline().boundingRect();
          if (source_image_rect.isValid() && current_image_rect.isValid()) {
            corrected_page_rect.translate((current_image_rect.width() - source_image_rect.width()) / 2,
                                          (current_image_rect.height() - source_image_rect.height()) / 2);
            new_params.setPageRect(corrected_page_rect);
          }
        }
      }
      if (new_params.contentDetectionMode() == MODE_MANUAL) {
        if (!apply_content_box) {
          new_params.setContentRect(old_params->contentRect());
        } else if (!new_params.contentRect().isEmpty()) {
          QRectF corrected_content_rect = new_params.contentRect();
          const QRectF& source_page_rect = m_uiData.pageRect();
          const QRectF& new_page_rect = new_params.pageRect();
          corrected_content_rect.translate(new_page_rect.x() - source_page_rect.x(),
                                           new_page_rect.y() - source_page_rect.y());
          new_params.setContentRect(corrected_content_rect);
        }
      }
    }

    m_settings->setPageParams(page_id, new_params);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }

  emit reloadRequested();
}  // OptionsWidget::applySelection


void OptionsWidget::updateUnits(Units units) {
  removeUiConnections();

  int decimals;
  double step;
  switch (units) {
    case PIXELS:
    case MILLIMETRES:
      decimals = 1;
      step = 1.0;
      break;
    default:
      decimals = 2;
      step = 0.01;
      break;
  }

  widthSpinBox->setDecimals(decimals);
  widthSpinBox->setSingleStep(step);
  heightSpinBox->setDecimals(decimals);
  heightSpinBox->setSingleStep(step);

  updatePageRectSize(m_uiData.pageRect().size());

  setupUiConnections();
}

#define CONNECT(...) m_connectionList.push_back(connect(__VA_ARGS__));

void OptionsWidget::setupUiConnections() {
  CONNECT(widthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(dimensionsChangedLocally(double)));
  CONNECT(heightSpinBox, SIGNAL(valueChanged(double)), this, SLOT(dimensionsChangedLocally(double)));
  CONNECT(contentDetectAutoBtn, &QPushButton::pressed, this,
          boost::bind(&OptionsWidget::contentDetectToggled, this, MODE_AUTO));
  CONNECT(contentDetectManualBtn, &QPushButton::pressed, this,
          boost::bind(&OptionsWidget::contentDetectToggled, this, MODE_MANUAL));
  CONNECT(contentDetectDisableBtn, &QPushButton::pressed, this,
          boost::bind(&OptionsWidget::contentDetectToggled, this, MODE_DISABLED));
  CONNECT(pageDetectAutoBtn, &QPushButton::pressed, this,
          boost::bind(&OptionsWidget::pageDetectToggled, this, MODE_AUTO));
  CONNECT(pageDetectManualBtn, &QPushButton::pressed, this,
          boost::bind(&OptionsWidget::pageDetectToggled, this, MODE_MANUAL));
  CONNECT(pageDetectDisableBtn, &QPushButton::pressed, this,
          boost::bind(&OptionsWidget::pageDetectToggled, this, MODE_DISABLED));
  CONNECT(fineTuneBtn, SIGNAL(toggled(bool)), this, SLOT(fineTuningChanged(bool)));
  CONNECT(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
}

#undef CONNECT

void OptionsWidget::removeUiConnections() {
  for (const auto& connection : m_connectionList) {
    disconnect(connection);
  }
  m_connectionList.clear();
}


/*========================= OptionsWidget::UiData ======================*/

OptionsWidget::UiData::UiData()
    : m_contentDetectionMode(MODE_AUTO), m_pageDetectionMode(MODE_DISABLED), m_fineTuneCornersEnabled(false) {}

OptionsWidget::UiData::~UiData() = default;

void OptionsWidget::UiData::setSizeCalc(const PhysSizeCalc& calc) {
  m_sizeCalc = calc;
}

void OptionsWidget::UiData::setContentRect(const QRectF& content_rect) {
  m_contentRect = content_rect;
}

const QRectF& OptionsWidget::UiData::contentRect() const {
  return m_contentRect;
}

void OptionsWidget::UiData::setPageRect(const QRectF& page_rect) {
  m_pageRect = page_rect;
}

const QRectF& OptionsWidget::UiData::pageRect() const {
  return m_pageRect;
}

QSizeF OptionsWidget::UiData::contentSizeMM() const {
  return m_sizeCalc.sizeMM(m_contentRect);
}

void OptionsWidget::UiData::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

const Dependencies& OptionsWidget::UiData::dependencies() const {
  return m_deps;
}

void OptionsWidget::UiData::setContentDetectionMode(AutoManualMode mode) {
  m_contentDetectionMode = mode;
}

AutoManualMode OptionsWidget::UiData::contentDetectionMode() const {
  return m_contentDetectionMode;
}

void OptionsWidget::UiData::setPageDetectionMode(AutoManualMode mode) {
  m_pageDetectionMode = mode;
}

AutoManualMode OptionsWidget::UiData::pageDetectionMode() const {
  return m_pageDetectionMode;
}
void OptionsWidget::UiData::setFineTuneCornersEnabled(bool fine_tune) {
  m_fineTuneCornersEnabled = fine_tune;
}

bool OptionsWidget::UiData::isFineTuningCornersEnabled() const {
  return m_fineTuneCornersEnabled;
}
}  // namespace select_content