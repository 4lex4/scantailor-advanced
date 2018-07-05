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

#include "Filter.h"
#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include <OrderByDeviationProvider.h>
#include <UnitsConverter.h>
#include <Utils.h>
#include <XmlMarshaller.h>
#include <XmlUnmarshaller.h>
#include <filters/page_layout/CacheDrivenTask.h>
#include <filters/page_layout/Task.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>
#include "CacheDrivenTask.h"
#include "CommandLine.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "OrderByHeightProvider.h"
#include "OrderByWidthProvider.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Task.h"

namespace select_content {
Filter::Filter(const PageSelectionAccessor& page_selection_accessor)
    : m_settings(new Settings), m_selectedPageOrder(0) {
  if (CommandLine::get().isGui()) {
    m_optionsWidget.reset(new OptionsWidget(m_settings, page_selection_accessor));
  }

  const PageOrderOption::ProviderPtr default_order;
  const auto order_by_width = make_intrusive<OrderByWidthProvider>(m_settings);
  const auto order_by_height = make_intrusive<OrderByHeightProvider>(m_settings);
  const auto order_by_deviation = make_intrusive<OrderByDeviationProvider>(m_settings->deviationProvider());
  m_pageOrderOptions.emplace_back(tr("Natural order"), default_order);
  m_pageOrderOptions.emplace_back(tr("Order by increasing width"), order_by_width);
  m_pageOrderOptions.emplace_back(tr("Order by increasing height"), order_by_height);
  m_pageOrderOptions.emplace_back(tr("Order by decreasing deviation"), order_by_deviation);
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

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) {
  m_optionsWidget->preUpdateUI(page_info);
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filter_el(doc.createElement("select-content"));

  filter_el.appendChild(XmlMarshaller(doc).sizeF(m_settings->pageDetectionBox(), "page-detection-box"));
  filter_el.setAttribute("pageDetectionTolerance", Utils::doubleToString(m_settings->pageDetectionTolerance()));

  writer.enumPages(
      [&](const PageId& page_id, int numeric_id) { this->writePageSettings(doc, filter_el, page_id, numeric_id); });

  return filter_el;
}

void Filter::writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const {
  const std::unique_ptr<Params> params(m_settings->getPageParams(page_id));
  if (!params) {
    return;
  }

  QDomElement page_el(doc.createElement("page"));
  page_el.setAttribute("id", numeric_id);
  page_el.appendChild(params->toXml(doc, "params"));

  filter_el.appendChild(page_el);
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filters_el) {
  m_settings->clear();

  const QDomElement filter_el(filters_el.namedItem("select-content").toElement());

  m_settings->setPageDetectionBox(XmlUnmarshaller::sizeF(filter_el.namedItem("page-detection-box").toElement()));
  m_settings->setPageDetectionTolerance(filter_el.attribute("pageDetectionTolerance", "0.1").toDouble());

  const QString page_tag_name("page");
  QDomNode node(filter_el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != page_tag_name) {
      continue;
    }
    const QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const PageId page_id(reader.pageId(id));
    if (page_id.isNull()) {
      continue;
    }

    const QDomElement params_el(el.namedItem("params").toElement());
    if (params_el.isNull()) {
      continue;
    }

    const Params params(params_el);
    m_settings->setPageParams(page_id, params);
  }
}  // Filter::loadSettings

intrusive_ptr<Task> Filter::createTask(const PageId& page_id,
                                       intrusive_ptr<page_layout::Task> next_task,
                                       bool batch,
                                       bool debug) {
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), std::move(next_task), m_settings, page_id, batch, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(intrusive_ptr<page_layout::CacheDrivenTask> next_task) {
  return make_intrusive<CacheDrivenTask>(m_settings, std::move(next_task));
}

void Filter::loadDefaultSettings(const PageInfo& page_info) {
  if (!m_settings->isParamsNull(page_info.id())) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
  const DefaultParams::SelectContentParams& selectContentParams = defaultParams.getSelectContentParams();

  const UnitsConverter unitsConverter(page_info.metadata().dpi());

  const QSizeF& pageRectSize = selectContentParams.getPageRectSize();
  double pageRectWidth = pageRectSize.width();
  double pageRectHeight = pageRectSize.height();
  unitsConverter.convert(pageRectWidth, pageRectHeight, defaultParams.getUnits(), PIXELS);

  m_settings->setPageParams(
      page_info.id(), Params(QRectF(), QSizeF(), QRectF(QPointF(0, 0), QSizeF(pageRectWidth, pageRectHeight)),
                             Dependencies(), selectContentParams.isContentDetectEnabled() ? MODE_AUTO : MODE_DISABLED,
                             selectContentParams.getPageDetectMode(), selectContentParams.isFineTuneCorners()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
}

}  // namespace select_content