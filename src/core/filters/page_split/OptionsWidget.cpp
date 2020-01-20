// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"

#include <core/IconProvider.h>

#include <cassert>
#include <utility>

#include "Filter.h"
#include "PageLayoutAdapter.h"
#include "ProjectPages.h"
#include "ScopedIncDec.h"
#include "Settings.h"
#include "SplitModeDialog.h"

namespace page_split {
OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings,
                             intrusive_ptr<ProjectPages> pageSequence,
                             const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::move(settings)),
      m_pages(std::move(pageSequence)),
      m_pageSelectionAccessor(pageSelectionAccessor),
      m_connectionManager(std::bind(&OptionsWidget::setupUiConnections, this)) {
  setupUi(this);
  setupIcons();

  // Workaround for QTBUG-182
  auto* grp = new QButtonGroup(this);
  grp->addButton(autoBtn);
  grp->addButton(manualBtn);

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageId& pageId) {
  auto block = m_connectionManager.getScopedBlock();

  m_pageId = pageId;
  const Settings::Record record(m_settings->getPageRecord(pageId.imageId()));
  const LayoutType layoutType(record.combinedLayoutType());

  switch (layoutType) {
    case AUTO_LAYOUT_TYPE:
      // Uncheck all buttons.  Can only be done
      // by playing with exclusiveness.
      twoPagesBtn->setChecked(true);
      twoPagesBtn->setAutoExclusive(false);
      twoPagesBtn->setChecked(false);
      twoPagesBtn->setAutoExclusive(true);
      break;
    case SINGLE_PAGE_UNCUT:
      singlePageUncutBtn->setChecked(true);
      break;
    case PAGE_PLUS_OFFCUT:
      pagePlusOffcutBtn->setChecked(true);
      break;
    case TWO_PAGES:
      twoPagesBtn->setChecked(true);
      break;
  }

  splitLineGroup->setVisible(layoutType != SINGLE_PAGE_UNCUT);

  if (layoutType == AUTO_LAYOUT_TYPE) {
    changeBtn->setEnabled(false);
    scopeLabel->setText("?");
  } else {
    changeBtn->setEnabled(true);
    scopeLabel->setText(tr("Set manually"));
  }

  // Uncheck both the Auto and Manual buttons.
  autoBtn->setChecked(true);
  autoBtn->setAutoExclusive(false);
  autoBtn->setChecked(false);
  autoBtn->setAutoExclusive(true);
  // And disable both of them.
  autoBtn->setEnabled(false);
  manualBtn->setEnabled(false);
}  // OptionsWidget::preUpdateUI

void OptionsWidget::postUpdateUI(const UiData& uiData) {
  auto block = m_connectionManager.getScopedBlock();

  m_uiData = uiData;

  changeBtn->setEnabled(true);
  autoBtn->setEnabled(true);
  manualBtn->setEnabled(true);

  if (uiData.splitLineMode() == MODE_AUTO) {
    autoBtn->setChecked(true);
  } else {
    manualBtn->setChecked(true);
  }

  const PageLayout::Type layoutType = uiData.pageLayout().type();

  switch (layoutType) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      singlePageUncutBtn->setChecked(true);
      break;
    case PageLayout::SINGLE_PAGE_CUT:
      pagePlusOffcutBtn->setChecked(true);
      break;
    case PageLayout::TWO_PAGES:
      twoPagesBtn->setChecked(true);
      break;
  }

  splitLineGroup->setVisible(layoutType != PageLayout::SINGLE_PAGE_UNCUT);

  if (uiData.layoutTypeAutoDetected()) {
    scopeLabel->setText(tr("Auto detected"));
  }
}  // OptionsWidget::postUpdateUI

void OptionsWidget::pageLayoutSetExternally(const PageLayout& pageLayout) {
  auto block = m_connectionManager.getScopedBlock();

  m_uiData.setPageLayout(pageLayout);
  m_uiData.setSplitLineMode(MODE_MANUAL);
  commitCurrentParams();

  manualBtn->setChecked(true);

  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::layoutTypeButtonToggled(const bool checked) {
  if (!checked) {
    return;
  }

  LayoutType lt;
  ProjectPages::LayoutType plt = ProjectPages::ONE_PAGE_LAYOUT;

  QObject* button = sender();
  if (button == singlePageUncutBtn) {
    lt = SINGLE_PAGE_UNCUT;
  } else if (button == pagePlusOffcutBtn) {
    lt = PAGE_PLUS_OFFCUT;
  } else {
    assert(button == twoPagesBtn);
    lt = TWO_PAGES;
    plt = ProjectPages::TWO_PAGE_LAYOUT;
  }

  Settings::UpdateAction update;
  update.setLayoutType(lt);

  splitLineGroup->setVisible(lt != SINGLE_PAGE_UNCUT);
  scopeLabel->setText(tr("Set manually"));

  m_pages->setLayoutTypeFor(m_pageId.imageId(), plt);

  if ((lt == PAGE_PLUS_OFFCUT) || ((lt != SINGLE_PAGE_UNCUT) && (m_uiData.splitLineMode() == MODE_AUTO))) {
    m_settings->updatePage(m_pageId.imageId(), update);
    emit reloadRequested();
  } else {
    PageLayout::Type plt;
    if (lt == SINGLE_PAGE_UNCUT) {
      plt = PageLayout::SINGLE_PAGE_UNCUT;
    } else {
      assert(lt == TWO_PAGES);
      plt = PageLayout::TWO_PAGES;
    }

    PageLayout newLayout(m_uiData.pageLayout());
    newLayout.setType(plt);
    const Params newParams(newLayout, m_uiData.dependencies(), m_uiData.splitLineMode());

    update.setParams(newParams);
    m_settings->updatePage(m_pageId.imageId(), update);

    m_uiData.setPageLayout(newLayout);

    emit pageLayoutSetLocally(newLayout);
    emit invalidateThumbnail(m_pageId);
  }
}  // OptionsWidget::layoutTypeButtonToggled

void OptionsWidget::showChangeDialog() {
  const Settings::Record record(m_settings->getPageRecord(m_pageId.imageId()));
  const Params* params = record.params();
  if (!params) {
    return;
  }

  auto* dialog = new SplitModeDialog(this, m_pageId, m_pageSelectionAccessor, record.combinedLayoutType(),
                                     params->pageLayout().type());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, SIGNAL(accepted(const std::set<PageId>&, LayoutType, bool)), this,
          SLOT(layoutTypeSet(const std::set<PageId>&, LayoutType, bool)));
  dialog->show();
}

void OptionsWidget::layoutTypeSet(const std::set<PageId>& pages, const LayoutType layoutType, bool applyCut) {
  if (pages.empty()) {
    return;
  }

  const Params params = *(m_settings->getPageRecord(m_pageId.imageId()).params());

  if (layoutType != AUTO_LAYOUT_TYPE) {
    for (const PageId& pageId : pages) {
      Settings::UpdateAction updateAction;
      updateAction.setLayoutType(layoutType);
      if (applyCut && (layoutType != SINGLE_PAGE_UNCUT)) {
        Params newPageParams(params);
        const Params* oldPageParams = m_settings->getPageRecord(pageId.imageId()).params();
        if (oldPageParams) {
          PageLayout newPageLayout = PageLayoutAdapter::adaptPageLayout(
              params.pageLayout(), oldPageParams->pageLayout().uncutOutline().boundingRect());

          updateAction.setLayoutType(newPageLayout.toLayoutType());
          newPageParams.setPageLayout(newPageLayout);

          Dependencies deps = oldPageParams->dependencies();
          deps.setLayoutType(newPageLayout.toLayoutType());
          newPageParams.setDependencies(deps);
        }
        updateAction.setParams(newPageParams);
      }
      m_settings->updatePage(pageId.imageId(), updateAction);
    }
  } else {
    for (const PageId& pageId : pages) {
      Settings::UpdateAction updateParams;
      updateParams.setLayoutType(layoutType);
      m_settings->updatePage(pageId.imageId(), updateParams);
    }
  }

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& pageId : pages) {
      emit invalidateThumbnail(pageId);
    }
  }

  if (layoutType == AUTO_LAYOUT_TYPE) {
    scopeLabel->setText(tr("Auto detected"));
    emit reloadRequested();
  } else {
    scopeLabel->setText(tr("Set manually"));
  }
}  // OptionsWidget::layoutTypeSet

void OptionsWidget::splitLineModeChanged(const bool autoMode) {
  if (autoMode) {
    Settings::UpdateAction update;
    update.clearParams();
    m_settings->updatePage(m_pageId.imageId(), update);
    m_uiData.setSplitLineMode(MODE_AUTO);
    emit reloadRequested();
  } else {
    m_uiData.setSplitLineMode(MODE_MANUAL);
    commitCurrentParams();
  }
}

void OptionsWidget::commitCurrentParams() {
  const Params params(m_uiData.pageLayout(), m_uiData.dependencies(), m_uiData.splitLineMode());
  Settings::UpdateAction update;
  update.setParams(params);
  m_settings->updatePage(m_pageId.imageId(), update);
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void OptionsWidget::setupUiConnections() {
  CONNECT(singlePageUncutBtn, SIGNAL(toggled(bool)), this, SLOT(layoutTypeButtonToggled(bool)));
  CONNECT(pagePlusOffcutBtn, SIGNAL(toggled(bool)), this, SLOT(layoutTypeButtonToggled(bool)));
  CONNECT(twoPagesBtn, SIGNAL(toggled(bool)), this, SLOT(layoutTypeButtonToggled(bool)));
  CONNECT(changeBtn, SIGNAL(clicked()), this, SLOT(showChangeDialog()));
  CONNECT(autoBtn, SIGNAL(toggled(bool)), this, SLOT(splitLineModeChanged(bool)));
}

#undef CONNECT

void OptionsWidget::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  singlePageUncutBtn->setIcon(iconProvider.getIcon("single_page_uncut"));
  pagePlusOffcutBtn->setIcon(iconProvider.getIcon("right_page_plus_offcut"));
  twoPagesBtn->setIcon(iconProvider.getIcon("two_pages"));
}

/*============================= Widget::UiData ==========================*/

OptionsWidget::UiData::UiData() : m_splitLineMode(MODE_AUTO), m_layoutTypeAutoDetected(false) {}

OptionsWidget::UiData::~UiData() = default;

void OptionsWidget::UiData::setPageLayout(const PageLayout& layout) {
  m_pageLayout = layout;
}

const PageLayout& OptionsWidget::UiData::pageLayout() const {
  return m_pageLayout;
}

void OptionsWidget::UiData::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

const Dependencies& OptionsWidget::UiData::dependencies() const {
  return m_deps;
}

void OptionsWidget::UiData::setSplitLineMode(const AutoManualMode mode) {
  m_splitLineMode = mode;
}

AutoManualMode OptionsWidget::UiData::splitLineMode() const {
  return m_splitLineMode;
}

bool OptionsWidget::UiData::layoutTypeAutoDetected() const {
  return m_layoutTypeAutoDetected;
}

void OptionsWidget::UiData::setLayoutTypeAutoDetected(const bool val) {
  m_layoutTypeAutoDetected = val;
}
}  // namespace page_split