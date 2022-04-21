// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"

#include <UnitsProvider.h>

#include <boost/bind/bind.hpp>
#include <utility>

#include "ApplyDialog.h"
#include "Settings.h"

namespace select_content {
OptionsWidget::OptionsWidget(std::shared_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::move(settings)),
      m_pageSelectionAccessor(pageSelectionAccessor),
      m_connectionManager(std::bind(&OptionsWidget::setupUiConnections, this)) {
  setupUi(this);

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageInfo& pageInfo) {
  auto block = m_connectionManager.getScopedBlock();

  m_pageId = pageInfo.id();
  m_dpi = pageInfo.metadata().dpi();

  contentBoxGroup->setEnabled(false);
  pageBoxGroup->setEnabled(false);

  pageDetectOptions->setVisible(false);
  fineTuneBtn->setVisible(false);
  dimensionsWidget->setVisible(false);

  onUnitsChanged(UnitsProvider::getInstance().getUnits());
}

void OptionsWidget::postUpdateUI(const UiData& uiData) {
  auto block = m_connectionManager.getScopedBlock();

  m_uiData = uiData;

  updateContentModeIndication(uiData.contentDetectionMode());
  updatePageModeIndication(uiData.pageDetectionMode());

  contentBoxGroup->setEnabled(true);
  pageBoxGroup->setEnabled(true);

  updatePageDetectOptionsDisplay();
  updatePageRectSize(m_uiData.pageRect().size());
}

void OptionsWidget::manualContentRectSet(const QRectF& contentRect) {
  m_uiData.setContentRect(contentRect);
  m_uiData.setContentDetectionMode(MODE_MANUAL);
  updateContentModeIndication(MODE_MANUAL);

  commitCurrentParams();

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::manualPageRectSet(const QRectF& pageRect) {
  m_uiData.setPageRect(pageRect);
  m_uiData.setPageDetectionMode(MODE_MANUAL);
  updatePageModeIndication(MODE_MANUAL);
  updatePageDetectOptionsDisplay();
  updatePageRectSize(pageRect.size());

  commitCurrentParams();

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::updatePageRectSize(const QSizeF& size) {
  auto block = m_connectionManager.getScopedBlock();

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
  const bool needUpdateState = ((mode == MODE_MANUAL) && (m_uiData.pageDetectionMode() == MODE_DISABLED));

  m_uiData.setPageDetectionMode(mode);
  updatePageDetectOptionsDisplay();
  commitCurrentParams();

  if (mode != MODE_MANUAL) {
    emit reloadRequested();
  } else if (needUpdateState) {
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

void OptionsWidget::applySelection(const std::set<PageId>& pages, const bool applyContentBox, const bool applyPageBox) {
  if (pages.empty()) {
    return;
  }

  const Params params(m_uiData.contentRect(), m_uiData.contentSizeMM(), m_uiData.pageRect(), Dependencies(),
                      m_uiData.contentDetectionMode(), m_uiData.pageDetectionMode(),
                      m_uiData.isFineTuningCornersEnabled());

  for (const PageId& pageId : pages) {
    if (m_pageId == pageId) {
      continue;
    }

    Params newParams(params);
    std::unique_ptr<Params> oldParams = m_settings->getPageParams(pageId);
    if (oldParams) {
      if (newParams.pageDetectionMode() == MODE_MANUAL) {
        if (!applyPageBox) {
          newParams.setPageRect(oldParams->pageRect());
        } else {
          QRectF correctedPageRect = newParams.pageRect();
          const QRectF sourceImageRect = newParams.dependencies().rotatedPageOutline().boundingRect();
          const QRectF currentImageRect = oldParams->dependencies().rotatedPageOutline().boundingRect();
          if (sourceImageRect.isValid() && currentImageRect.isValid()) {
            correctedPageRect.translate((currentImageRect.width() - sourceImageRect.width()) / 2,
                                        (currentImageRect.height() - sourceImageRect.height()) / 2);
            newParams.setPageRect(correctedPageRect);
          }
        }
      }
      if (newParams.contentDetectionMode() == MODE_MANUAL) {
        if (!applyContentBox) {
          newParams.setContentRect(oldParams->contentRect());
        } else if (!newParams.contentRect().isEmpty()) {
          QRectF correctedContentRect = newParams.contentRect();
          const QRectF& sourcePageRect = m_uiData.pageRect();
          const QRectF& newPageRect = newParams.pageRect();
          correctedContentRect.translate(newPageRect.x() - sourcePageRect.x(), newPageRect.y() - sourcePageRect.y());
          newParams.setContentRect(correctedContentRect);
        }
      }
    }

    m_settings->setPageParams(pageId, newParams);
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& pageId : pages) {
      emit invalidateThumbnail(pageId);
    }
  }

  emit reloadRequested();
}  // OptionsWidget::applySelection


void OptionsWidget::onUnitsChanged(Units units) {
  auto block = m_connectionManager.getScopedBlock();

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
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

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


/*========================= OptionsWidget::UiData ======================*/

OptionsWidget::UiData::UiData()
    : m_contentDetectionMode(MODE_AUTO), m_pageDetectionMode(MODE_DISABLED), m_fineTuneCornersEnabled(false) {}

OptionsWidget::UiData::~UiData() = default;
}  // namespace select_content
