// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"

#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include <OrderByDeviationProvider.h>
#include <UnitsConverter.h>
#include <filters/output/CacheDrivenTask.h>
#include <filters/output/Task.h>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>

#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "Guide.h"
#include "OptionsWidget.h"
#include "OrderByHeightProvider.h"
#include "OrderByWidthProvider.h"
#include "Params.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"
#include "Utils.h"

namespace page_layout {
Filter::Filter(intrusive_ptr<ProjectPages> pages, const PageSelectionAccessor& pageSelectionAccessor)
    : m_pages(std::move(pages)), m_settings(new Settings), m_selectedPageOrder(0) {
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
  return tr("Margins");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

void Filter::selected() {
  m_settings->removePagesMissingFrom(m_pages->toPageSequence(getView()));
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
  const Margins marginsMm(m_settings->getHardMarginsMM(pageInfo.id()));
  const Alignment alignment(m_settings->getPageAlignment(pageInfo.id()));
  m_optionsWidget->preUpdateUI(pageInfo, marginsMm, alignment);
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl(doc.createElement("page-layout"));

  filterEl.setAttribute("showMiddleRect", m_settings->isShowingMiddleRectEnabled() ? "1" : "0");

  if (!m_settings->guides().empty()) {
    QDomElement guidesEl(doc.createElement("guides"));
    for (const Guide& guide : m_settings->guides()) {
      guidesEl.appendChild(guide.toXml(doc, "guide"));
    }
    filterEl.appendChild(guidesEl);
  }

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

  const QDomElement filterEl(filtersEl.namedItem("page-layout").toElement());

  m_settings->enableShowingMiddleRect(filterEl.attribute("showMiddleRect") == "1");

  const QDomElement guidesEl = filterEl.namedItem("guides").toElement();
  if (!guidesEl.isNull()) {
    QDomNode node(guidesEl.firstChild());
    for (; !node.isNull(); node = node.nextSibling()) {
      if (!node.isElement() || (node.nodeName() != "guide")) {
        continue;
      }
      m_settings->guides().emplace_back(node.toElement());
    }
  }

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

void Filter::setContentBox(const PageId& pageId, const ImageTransformation& xform, const QRectF& contentRect) {
  const QSizeF contentSizeMm(Utils::calcRectSizeMM(xform, contentRect));
  m_settings->setContentSizeMM(pageId, contentSizeMm);
}

void Filter::invalidateContentBox(const PageId& pageId) {
  m_settings->invalidateContentSize(pageId);
}

bool Filter::checkReadyForOutput(const ProjectPages& pages, const PageId* ignore) {
  const PageSequence snapshot(pages.toPageSequence(PAGE_VIEW));
  return m_settings->checkEverythingDefined(snapshot, ignore);
}

intrusive_ptr<Task> Filter::createTask(const PageId& pageId,
                                       intrusive_ptr<output::Task> nextTask,
                                       const bool batch,
                                       const bool debug) {
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), std::move(nextTask), m_settings, pageId, batch, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(intrusive_ptr<output::CacheDrivenTask> nextTask) {
  return make_intrusive<CacheDrivenTask>(std::move(nextTask), m_settings);
}

void Filter::loadDefaultSettings(const PageInfo& pageInfo) {
  if (!m_settings->isParamsNull(pageInfo.id())) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::PageLayoutParams& pageLayoutParams = defaultParams.getPageLayoutParams();

  const UnitsConverter unitsConverter(pageInfo.metadata().dpi());

  const Margins& margins = pageLayoutParams.getHardMargins();
  double leftMargin = margins.left();
  double topMargin = margins.top();
  double rightMargin = margins.right();
  double bottomMargin = margins.bottom();
  unitsConverter.convert(leftMargin, topMargin, defaultParams.getUnits(), MILLIMETRES);
  unitsConverter.convert(rightMargin, bottomMargin, defaultParams.getUnits(), MILLIMETRES);

  m_settings->setPageParams(
      pageInfo.id(), Params(Margins(leftMargin, topMargin, rightMargin, bottomMargin), QRectF(), QRectF(), QSizeF(),
                            pageLayoutParams.getAlignment(), pageLayoutParams.isAutoMargins()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
}
}  // namespace page_layout