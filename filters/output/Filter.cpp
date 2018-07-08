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
#include <OrderByCompletenessProvider.h>
#include <tiff.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>
#include "CacheDrivenTask.h"
#include "CommandLine.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"
#include "ThumbnailPixmapCache.h"

namespace output {
Filter::Filter(const PageSelectionAccessor& page_selection_accessor)
    : m_settings(new Settings), m_selectedPageOrder(0) {
  if (CommandLine::get().isGui()) {
    m_optionsWidget.reset(new OptionsWidget(m_settings, page_selection_accessor));
  }

  const PageOrderOption::ProviderPtr default_order;
  const auto order_by_completeness = make_intrusive<OrderByCompletenessProvider>();
  m_pageOrderOptions.emplace_back(tr("Natural order"), default_order);
  m_pageOrderOptions.emplace_back(tr("Order by completeness"), order_by_completeness);
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("output::Filter", "Output");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_settings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) {
  m_optionsWidget->preUpdateUI(page_info.id());
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filter_el(doc.createElement("output"));

  writer.enumPages(
      [&](const PageId& page_id, int numeric_id) { this->writePageSettings(doc, filter_el, page_id, numeric_id); });

  return filter_el;
}

void Filter::writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const {
  const Params params(m_settings->getParams(page_id));

  QDomElement page_el(doc.createElement("page"));
  page_el.setAttribute("id", numeric_id);

  page_el.appendChild(m_settings->pictureZonesForPage(page_id).toXml(doc, "zones"));
  page_el.appendChild(m_settings->fillZonesForPage(page_id).toXml(doc, "fill-zones"));
  page_el.appendChild(params.toXml(doc, "params"));
  page_el.appendChild(m_settings->getOutputProcessingParams(page_id).toXml(doc, "processing-params"));

  std::unique_ptr<OutputParams> output_params(m_settings->getOutputParams(page_id));
  if (output_params) {
    page_el.appendChild(output_params->toXml(doc, "output-params"));
  }

  filter_el.appendChild(page_el);
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filters_el) {
  m_settings->clear();

  const QDomElement filter_el(filters_el.namedItem("output").toElement());

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

    const ZoneSet picture_zones(el.namedItem("zones").toElement(), m_pictureZonePropFactory);
    if (!picture_zones.empty()) {
      m_settings->setPictureZones(page_id, picture_zones);
    }

    const ZoneSet fill_zones(el.namedItem("fill-zones").toElement(), m_fillZonePropFactory);
    if (!fill_zones.empty()) {
      m_settings->setFillZones(page_id, fill_zones);
    }

    const QDomElement params_el(el.namedItem("params").toElement());
    if (!params_el.isNull()) {
      const Params params(params_el);
      m_settings->setParams(page_id, params);
    }

    const QDomElement output_processing_params_el(el.namedItem("processing-params").toElement());
    if (!output_processing_params_el.isNull()) {
      const OutputProcessingParams output_processing_params(output_processing_params_el);
      m_settings->setOutputProcessingParams(page_id, output_processing_params);
    }

    const QDomElement output_params_el(el.namedItem("output-params").toElement());
    if (!output_params_el.isNull()) {
      const OutputParams output_params(output_params_el);
      m_settings->setOutputParams(page_id, output_params);
    }
  }
}  // Filter::loadSettings

intrusive_ptr<Task> Filter::createTask(const PageId& page_id,
                                       intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                                       const OutputFileNameGenerator& out_file_name_gen,
                                       const bool batch,
                                       const bool debug) {
  ImageViewTab lastTab(TAB_OUTPUT);
  if (m_optionsWidget.get() != nullptr) {
    lastTab = m_optionsWidget->lastTab();
  }

  return make_intrusive<Task>(intrusive_ptr<Filter>(this), m_settings, std::move(thumbnail_cache), page_id,
                              out_file_name_gen, lastTab, batch, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(const OutputFileNameGenerator& out_file_name_gen) {
  return make_intrusive<CacheDrivenTask>(m_settings, out_file_name_gen);
}

void Filter::loadDefaultSettings(const PageInfo& page_info) {
  if (!m_settings->isParamsNull(page_info.id())) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
  const DefaultParams::OutputParams& outputParams = defaultParams.getOutputParams();

  m_settings->setParams(
      page_info.id(),
      Params(outputParams.getDpi(), outputParams.getColorParams(), outputParams.getSplittingOptions(),
             outputParams.getPictureShapeOptions(), dewarping::DistortionModel(), outputParams.getDepthPerception(),
             outputParams.getDewarpingOptions(), outputParams.getDespeckleLevel()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
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
}  // namespace output