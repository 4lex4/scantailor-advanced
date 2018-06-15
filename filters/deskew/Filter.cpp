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
#include <CommandLine.h>
#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include <OrderByDeviationProvider.h>
#include <filters/select_content/CacheDrivenTask.h>
#include <filters/select_content/Task.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>
#include "AbstractRelinker.h"
#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Task.h"

namespace deskew {
Filter::Filter(const PageSelectionAccessor& page_selection_accessor)
    : m_ptrSettings(new Settings), m_ptrImageSettings(new ImageSettings), m_selectedPageOrder(0) {
  if (CommandLine::get().isGui()) {
    m_ptrOptionsWidget.reset(new OptionsWidget(m_ptrSettings, page_selection_accessor));
  }

  typedef PageOrderOption::ProviderPtr ProviderPtr;

  const ProviderPtr default_order;
  const auto order_by_deviation = make_intrusive<OrderByDeviationProvider>(m_ptrSettings->deviationProvider());
  m_pageOrderOptions.emplace_back(tr("Natural order"), default_order);
  m_pageOrderOptions.emplace_back(tr("Order by decreasing deviation"), order_by_deviation);
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("deskew::Filter", "Deskew");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_ptrSettings->performRelinking(relinker);
  m_ptrImageSettings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* const ui, const PageInfo& page_info) {
  m_ptrOptionsWidget->preUpdateUI(page_info.id());
  ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filter_el(doc.createElement("deskew"));

  writer.enumPages(
      [&](const PageId& page_id, const int numeric_id) { this->writeParams(doc, filter_el, page_id, numeric_id); });

  saveImageSettings(writer, doc, filter_el);

  return filter_el;
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filters_el) {
  m_ptrSettings->clear();

  const QDomElement filter_el(filters_el.namedItem("deskew").toElement());

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
    m_ptrSettings->setPageParams(page_id, params);
  }

  loadImageSettings(reader, filter_el.namedItem("image-settings").toElement());
}  // Filter::loadSettings

void Filter::writeParams(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const {
  const std::unique_ptr<Params> params(m_ptrSettings->getPageParams(page_id));
  if (!params) {
    return;
  }

  QDomElement page_el(doc.createElement("page"));
  page_el.setAttribute("id", numeric_id);
  page_el.appendChild(params->toXml(doc, "params"));

  filter_el.appendChild(page_el);
}

intrusive_ptr<Task> Filter::createTask(const PageId& page_id,
                                       intrusive_ptr<select_content::Task> next_task,
                                       const bool batch_processing,
                                       const bool debug) {
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), m_ptrSettings, m_ptrImageSettings, std::move(next_task),
                              page_id, batch_processing, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(intrusive_ptr<select_content::CacheDrivenTask> next_task) {
  return make_intrusive<CacheDrivenTask>(m_ptrSettings, std::move(next_task));
}

std::vector<PageOrderOption> Filter::pageOrderOptions() const {
  return m_pageOrderOptions;
}

int Filter::selectedPageOrder() const {
  return m_selectedPageOrder;
}

void Filter::selectPageOrder(int option) {
  assert((unsigned) option < m_pageOrderOptions.size());
  m_selectedPageOrder = option;
}

void Filter::loadDefaultSettings(const PageInfo& page_info) {
  if (!m_ptrSettings->isParamsNull(page_info.id())) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
  const DefaultParams::DeskewParams& deskewParams = defaultParams.getDeskewParams();

  m_ptrSettings->setPageParams(page_info.id(),
                               Params(deskewParams.getDeskewAngleDeg(), Dependencies(), deskewParams.getMode()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_ptrOptionsWidget.get();
}

void Filter::saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filter_el) const {
  QDomElement image_settings_el(doc.createElement("image-settings"));
  writer.enumPages([&](const PageId& page_id, const int numeric_id) {
    this->writeImageParams(doc, image_settings_el, page_id, numeric_id);
  });

  filter_el.appendChild(image_settings_el);
}

void Filter::writeImageParams(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const {
  const std::unique_ptr<ImageSettings::PageParams> params(m_ptrImageSettings->getPageParams(page_id));
  if (!params) {
    return;
  }

  QDomElement page_el(doc.createElement("page"));
  page_el.setAttribute("id", numeric_id);
  page_el.appendChild(params->toXml(doc, "image-params"));

  filter_el.appendChild(page_el);
}

void Filter::loadImageSettings(const ProjectReader& reader, const QDomElement& image_settings_el) {
  m_ptrImageSettings->clear();

  const QString page_tag_name("page");
  QDomNode node(image_settings_el.firstChild());
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

    const QDomElement params_el(el.namedItem("image-params").toElement());
    if (params_el.isNull()) {
      continue;
    }

    const ImageSettings::PageParams params(params_el);
    m_ptrImageSettings->setPageParams(page_id, params);
  }
}
}  // namespace deskew