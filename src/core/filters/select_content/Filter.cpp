// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"
#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include <OrderByDeviationProvider.h>
#include <UnitsConverter.h>
#include <XmlMarshaller.h>
#include <XmlUnmarshaller.h>
#include <filters/page_layout/CacheDrivenTask.h>
#include <filters/page_layout/Task.h>
#include <foundation/Utils.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>
#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "OrderByHeightProvider.h"
#include "OrderByWidthProvider.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Task.h"

using namespace foundation;

namespace select_content {
Filter::Filter(const PageSelectionAccessor& pageSelectionAccessor) : m_settings(new Settings), m_selectedPageOrder(0) {
  m_optionsWidget.reset(new OptionsWidget(m_settings, pageSelectionAccessor));

  const PageOrderOption::ProviderPtr defaultOrder;
  const auto orderByWidth = make_intrusive<OrderByWidthProvider>(m_settings);
  const auto orderByHeight = make_intrusive<OrderByHeightProvider>(m_settings);
  const auto orderByDeviation = make_intrusive<OrderByDeviationProvider>(m_settings->deviationProvider());
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
  filterEl.setAttribute("pageDetectionTolerance", Utils::doubleToString(m_settings->pageDetectionTolerance()));

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

intrusive_ptr<Task> Filter::createTask(const PageId& pageId,
                                       intrusive_ptr<page_layout::Task> nextTask,
                                       bool batch,
                                       bool debug) {
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), std::move(nextTask), m_settings, pageId, batch, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(intrusive_ptr<page_layout::CacheDrivenTask> nextTask) {
  return make_intrusive<CacheDrivenTask>(m_settings, std::move(nextTask));
}

void Filter::loadDefaultSettings(const PageInfo& pageInfo) {
  if (!m_settings->isParamsNull(pageInfo.id())) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::SelectContentParams& selectContentParams = defaultParams.getSelectContentParams();

  const UnitsConverter unitsConverter(pageInfo.metadata().dpi());

  const QSizeF& pageRectSize = selectContentParams.getPageRectSize();
  double pageRectWidth = pageRectSize.width();
  double pageRectHeight = pageRectSize.height();
  unitsConverter.convert(pageRectWidth, pageRectHeight, defaultParams.getUnits(), PIXELS);

  m_settings->setPageParams(
      pageInfo.id(), Params(QRectF(), QSizeF(), QRectF(QPointF(0, 0), QSizeF(pageRectWidth, pageRectHeight)),
                            Dependencies(), selectContentParams.isContentDetectEnabled() ? MODE_AUTO : MODE_DISABLED,
                            selectContentParams.getPageDetectMode(), selectContentParams.isFineTuneCorners()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
}

}  // namespace select_content