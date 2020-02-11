// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"

#include <OrderByDeviationProvider.h>
#include <XmlMarshaller.h>
#include <XmlUnmarshaller.h>
#include <filters/page_layout/CacheDrivenTask.h>
#include <filters/page_layout/Task.h>
#include <foundation/Utils.h>

#include <utility>

#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "OrderByHeightProvider.h"
#include "OrderByWidthProvider.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Task.h"
#include "Utils.h"

using namespace foundation;

namespace select_content {
Filter::Filter(const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::make_shared<Settings>()), m_selectedPageOrder(0) {
  m_optionsWidget.reset(new OptionsWidget(m_settings, pageSelectionAccessor));

  const PageOrderOption::ProviderPtr defaultOrder;
  const auto orderByWidth = std::make_shared<OrderByWidthProvider>(m_settings);
  const auto orderByHeight = std::make_shared<OrderByHeightProvider>(m_settings);
  const auto orderByDeviation = std::make_shared<OrderByDeviationProvider>(m_settings->deviationProvider());
  m_pageOrderOptions.emplace_back(tr("Natural order"), defaultOrder);
  m_pageOrderOptions.emplace_back(tr("Order by increasing width"), orderByWidth);
  m_pageOrderOptions.emplace_back(tr("Order by increasing height"), orderByHeight);
  m_pageOrderOptions.emplace_back(tr("Order by decreasing deviation"), orderByDeviation);
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return tr("Select Content");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

int Filter::selectedPageOrder() const {
  return m_selectedPageOrder;
}

void Filter::selectPageOrder(int option) {
  assert((unsigned) option < m_pageOrderOptions.size());
  m_selectedPageOrder = option;
}

std::vector<PageOrderOption> Filter::pageOrderOptions() const {
  return m_pageOrderOptions;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_settings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) {
  m_optionsWidget->preUpdateUI(pageInfo);
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl(doc.createElement("select-content"));

  filterEl.appendChild(XmlMarshaller(doc).sizeF(m_settings->pageDetectionBox(), "page-detection-box"));
  filterEl.setAttribute("pageDetectionTolerance",
                        foundation::Utils::doubleToString(m_settings->pageDetectionTolerance()));

  writer.enumPages(
      [&](const PageId& pageId, int numericId) { this->writePageSettings(doc, filterEl, pageId, numericId); });
  return filterEl;
}

void Filter::writePageSettings(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const std::unique_ptr<Params> params(m_settings->getPageParams(pageId));
  if (!params) {
    return;
  }

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);
  pageEl.appendChild(params->toXml(doc, "params"));

  filterEl.appendChild(pageEl);
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) {
  m_settings->clear();

  const QDomElement filterEl(filtersEl.namedItem("select-content").toElement());

  m_settings->setPageDetectionBox(XmlUnmarshaller::sizeF(filterEl.namedItem("page-detection-box").toElement()));
  m_settings->setPageDetectionTolerance(filterEl.attribute("pageDetectionTolerance", "0.1").toDouble());

  const QString pageTagName("page");
  QDomNode node(filterEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != pageTagName) {
      continue;
    }
    const QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const PageId pageId(reader.pageId(id));
    if (pageId.isNull()) {
      continue;
    }

    const QDomElement paramsEl(el.namedItem("params").toElement());
    if (paramsEl.isNull()) {
      continue;
    }

    const Params params(paramsEl);
    m_settings->setPageParams(pageId, params);
  }
}  // Filter::loadSettings

std::shared_ptr<Task> Filter::createTask(const PageId& pageId,
                                         std::shared_ptr<page_layout::Task> nextTask,
                                         bool batch,
                                         bool debug) {
  return std::make_shared<Task>(std::static_pointer_cast<Filter>(shared_from_this()), std::move(nextTask), m_settings,
                                pageId, batch, debug);
}

std::shared_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(std::shared_ptr<page_layout::CacheDrivenTask> nextTask) {
  return std::make_shared<CacheDrivenTask>(m_settings, std::move(nextTask));
}

void Filter::loadDefaultSettings(const PageInfo& pageInfo) {
  if (!m_settings->isParamsNull(pageInfo.id()))
    return;

  m_settings->setPageParams(pageInfo.id(), select_content::Utils::buildDefaultParams(pageInfo.metadata().dpi()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
}

}  // namespace select_content